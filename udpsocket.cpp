//
//  Created by Jean-Luc Deltombe (LX3JL) on 31/10/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020,2025 Thomas A. Early, N7TAE
//
// ----------------------------------------------------------------------------
//    This file is part of mrefd.
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    with this software.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <string.h>
#include <csignal>

#include "udpsocket.h"
#include "defines.h"

////////////////////////////////////////////////////////////////////////////////////////
// constructor

CUdpSocket::CUdpSocket() : m_fd(-1) {}

////////////////////////////////////////////////////////////////////////////////////////
// destructor

CUdpSocket::~CUdpSocket()
{
	Close();
}

////////////////////////////////////////////////////////////////////////////////////////
// open & close

// returns true on error
bool CUdpSocket::Open(const CIp &Ip)
{
	// check for a valid family
	if (AF_UNSPEC == Ip.GetFamily())
		return true;

	// create socket
	m_fd = socket(Ip.GetFamily(), SOCK_DGRAM, 0);
	if ( m_fd < 0 )
	{
		std::cerr << "Unable to open socket on " << Ip << ", " << strerror(errno) << std::endl;
		return true;
	}
	// initialize sockaddr struct
	m_addr = Ip;

	int reuse = 1;
	if ( 0 > setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)))
	{
		std::cerr << "Cannot set the UDP socket option on " << m_addr << ", " << strerror(errno) << std::endl;
		Close();
		return true;
	}

	if (fcntl(m_fd, F_SETFL, O_NONBLOCK))
	{
		std::cerr << "fcntl set non-blocking failed on " << m_addr << ", " << strerror(errno) << std::endl;
		Close();
		return true;
	}

	if ( bind(m_fd, m_addr.GetCPointer(), m_addr.GetSize()) )
	{
		std::cerr << "bind failed on " << m_addr << ", " << strerror(errno) << std::endl;
		Close();
		return true;
	}

	if (0 == m_addr.GetPort())  	// get the assigned port for an ephemeral port request
	{
		CIp a;
		socklen_t len = sizeof(struct sockaddr_storage);
		if (getsockname(m_fd, a.GetPointer(), &len))
		{
			std::cerr << "getsockname error " << m_addr << ", " << strerror(errno) << std::endl;
			Close();
			return true;
		}
		if (a != m_addr)
			std::cout << "getsockname didn't return the same address as set: returned " << a << ", should have been " << m_addr << std::endl;

		m_addr.SetPort(a.GetPort());
	}

	// done
	return false;
}

void CUdpSocket::Close(void)
{
	if ( m_fd >= 0 )
	{
		close(m_fd);
		m_fd = -1;
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// read

unsigned CUdpSocket::Receive(uint8_t *Buffer, CIp &Ip, int timeout)
{
	// socket valid ?
	if ( 0 > m_fd )
		return false;

	// control socket
	fd_set FdSet;
	FD_ZERO(&FdSet);
	FD_SET(m_fd, &FdSet);
	struct timeval tv;
	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;

	auto rval = select(m_fd + 1, &FdSet, 0, 0, &tv);
	if (rval > 0)
		return ReceiveFrom(Buffer, Ip);

	if (rval < 0)
	{
		std::cerr << "select error on UPD port " << m_addr << ": " << strerror(errno) << std::endl;
		raise(SIGINT);
	}

	return 0u;
}

unsigned CUdpSocket::ReceiveFrom(uint8_t *Buffer, CIp &ip)
{
	// read
	socklen_t fromsize = sizeof(struct sockaddr_storage);
	auto rv = recvfrom(m_fd, Buffer, MAX_PACKET_SIZE, 0, ip.GetPointer(), &fromsize);
	if (rv < 0)
	{
		std::cerr << "ERROR ReceiveFrom():" << strerror(errno) << std::endl;
		raise(SIGINT);
		return 0u;
	}
	return unsigned(rv);
}

////////////////////////////////////////////////////////////////////////////////////////
// write

unsigned CUdpSocket::Send(const uint8_t *Buffer, size_t size, const CIp &Ip) const
{
	auto rv = sendto(m_fd, Buffer, size, 0, Ip.GetCPointer(), Ip.GetSize());
	if (rv < 0)
	{
		std::cerr << "ERROR sending " << size << " byte packet to " << Ip << ": " << strerror(errno) << std::endl;
		raise(SIGINT);
		return 0u;
	}
	return unsigned(rv);
}
