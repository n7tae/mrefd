/*
 *   Copyright (c) 2022 by Thomas A. Early N7TAE
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <ctype.h>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdint>
#include <poll.h>

#include "ip.h"
#include "udpsocket.h"
#include "callsign.h"

static void Dump(const char *title, const void *pointer, int length)
{
	const unsigned char *data = (const unsigned char *)pointer;

	std::cout << title << std::endl;

	unsigned int offset = 0U;

	while (length > 0) {

		unsigned int bytes = (length > 16) ? 16U : length;

		for (unsigned i = 0U; i < bytes; i++) {
			if (i)
				std::cout << " ";
			std::cout << std::hex << std::setw(2) << std::right << std::setfill('0') << int(data[offset + i]) << std::dec;
		}

		for (unsigned int i = bytes; i < 16U; i++)
			std::cout << "   ";

		std::cout << "   *";

		for (unsigned i = 0U; i < bytes; i++) {
			unsigned char c = data[offset + i];

			if (::isprint(c))
				std::cout << c;
			else
				std::cout << '.';
		}

		std::cout << '*' << std::endl;

		offset += 16U;

		if (length >= 16)
			length -= 16;
		else
			length = 0;
	}
}

struct SHost
{
	SHost(std::string u, std::string ip4, std::string ip6, std::string p) : url(u), ip4addr(ip4), ip6addr(ip6), port(std::stoul(p)) {}
	std::string url, ip4addr, ip6addr;
	uint16_t port;
};

int main(int argc, char *argv[])
{
	if (argc < 3 || argc > 4)
	{
		std::cout << "usage: " << argv[0] << "IpAddress port <module>" << std::endl;
		return EXIT_FAILURE;
	}

	unsigned port = std::stoul(argv[2]);
	if (1024 > port || port > 49000)
	{
		std::cerr << "port value " << port << " is out of range" << std::endl;
		return EXIT_FAILURE;
	}

	CIp bindaddr, toaddr(argv[1], AF_UNSPEC, SOCK_DGRAM, port);
	bindaddr.Initialize(toaddr.GetFamily(), port, "any");
	if (! bindaddr.IsSet() || ! toaddr.IsSet())
	{
		std::cerr << "Trouble with setting the binding addresses" << bindaddr << " and " << toaddr << std::endl;
		std::cerr << "Family is " << toaddr.GetFamily() << std::endl;
		return EXIT_FAILURE;
	}

	char mod;
	if (3 == argc)
	{
		mod = ' ';
	}
	else
	{
		mod = argv[3][0];
		if ('a'<=mod && mod<='z')
			mod = mod - 'a' + 'A';
		if ('A'>mod || mod>'Z')
		{
			std::cerr << "The optional module must be a letter and not '" << mod << "'" << std::endl;
			return EXIT_FAILURE;
		}
	}

	CUdpSocket usock;
	if (! usock.Open(bindaddr))
		return EXIT_FAILURE;

	struct pollfd pfd;
	pfd.fd = usock.GetSocket();
	pfd.events = POLLIN;
	pfd.revents = 0;

	uint8_t buf[UDP_BUFFER_LENMAX];
	memcpy(buf, "INFO", 4);
	buf[4] = mod;
	usock.Send(buf, 5, toaddr);

	auto rv = poll(&pfd, 1, 5000);
	if (rv < 0)
	{
		perror("poll");
		return EXIT_FAILURE;
	}
	if (rv == 0)
	{
		std::cerr << "No response from " << toaddr << std::endl;
		return EXIT_FAILURE;
	}

	CIp fromaddr;
	auto n = usock.ReceiveFrom(buf, fromaddr);
	if (n < 0)
	{
		perror("recvfrom");
		return EXIT_FAILURE;
	}
	if (n == 0)
	{
		std::cerr << "Received zero bytes from " << fromaddr << std::endl;
		return EXIT_FAILURE;
	}
	usock.Close();

	if (' ' == mod)
	{
		std::cout << "Received from " << fromaddr << " for reflector:" << std::endl;
		if (11 == n)
		{
			unsigned count = buf[4] * 0x100u + buf[5];
			std::cout << count << " clients" << std::endl;
			std::time_t t = 0;

			for (unsigned i=6; i<11; i++)
				t = t * 0x100u + buf[i];

			std::cout << "Global last-heard time: ";
			if (t)
			{
				char s[100];
				if (std::strftime(s, sizeof(s), "%F %T local time", std::localtime(&t)))
					std::cout << s << std::endl;
				else
					std::cout << "Unknow time: " << int64_t(t) << std::endl;
			}
			else
				std::cout << "nothing heard" << std::endl;
		}
		else
		{
			Dump("Unexpected response:", buf, n);
			return EXIT_FAILURE;
		}
	}
	else
	{
		std::cout << "Received from " << fromaddr << " for module " << mod << ":" << std::endl;

		if (5 == n && 0==memcmp(buf, "INFO?", 5))
		{
			std::cout << "Module is not configured." << std::endl;
			return EXIT_SUCCESS;
		}
		else if (18 == n)
		{
			unsigned count = buf[4] * 0x100u + buf[5];
			std::cout << count << " clients" << std::endl;

			std::cout << "This module is " << (bool(buf[6]) ? "" : "not ") << "encrypted" << std::endl;

			std::time_t t = 0;
			for (unsigned i=13; i<18; i++)
				t = t * 0x100u + buf[i];
			std::cout << "Last-heard: ";
			if (t)
			{
				CCallsign cs(buf+7);
				std::cout << cs << " at ";
				char s[100];
				if (std::strftime(s, sizeof(s), "%F %T local time", std::localtime(&t)))
					std::cout << s << std::endl;
				else
					std::cout << "Unknown time: " << int64_t(t) << std::endl;
			}
			else
				std::cout << "nothing heard" << std::endl;
		}
		else
		{
			Dump("Unexpected response:", buf, n);
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}
