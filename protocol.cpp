//
//  cprotocol.cpp
//  mrefd
//
//  Created by Jean-Luc Deltombe (LX3JL) on 01/11/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020 Thomas A. Early, N7TAE
//
// ----------------------------------------------------------------------------
//    This file is part of mrefd.
//
//    mrefd is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    mrefd is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include "main.h"
#include "protocol.h"
#include "clients.h"
#include "reflector.h"


////////////////////////////////////////////////////////////////////////////////////////
// constructor


CProtocol::CProtocol() : keep_running(true) {}


////////////////////////////////////////////////////////////////////////////////////////
// destructor

CProtocol::~CProtocol()
{
	// kill threads
	Close();

	// empty queue
	m_Queue.Lock();
	while ( !m_Queue.empty() )
	{
		m_Queue.pop();
	}
	m_Queue.Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////
// initialization

bool CProtocol::Initialize(const uint16_t port, const bool has_ipv4, const bool has_ipv6)
{
	// init reflector apparent callsign
	m_ReflectorCallsign = g_Reflector.GetCallsign();

	// reset stop flag
	keep_running = true;

	// create our sockets
#ifdef LISTEN_IPV4
	if (has_ipv4)
	{
		CIp ip4(AF_INET, port, LISTEN_IPV4);
		if ( ip4.IsSet() )
		{
			if (! m_Socket4.Open(ip4))
				return false;
		}
		std::cout << "Listening on " << ip4 << std::endl;
	}
#endif

#ifdef LISTEN_IPV6
	if (has_ipv6)
	{
		CIp ip6(AF_INET6, port, LISTEN_IPV6);
		if ( ip6.IsSet() )
		{
			if (! m_Socket6.Open(ip6))
			{
				m_Socket4.Close();
				return false;
			}
			std::cout << "Listening on " << ip6 << std::endl;
		}
	}
#endif

	try {
		m_Future = std::async(std::launch::async, &CProtocol::Thread, this);
	}
	catch (const std::exception &e)
	{
		std::cerr << "Could not start protocol on port " << port << ": " << e.what() << std::endl;
		m_Socket4.Close();
		m_Socket6.Close();
		return false;
	}

	return true;
}

void CProtocol::Thread()
{
	while (keep_running)
	{
		Task();
	}
}

void CProtocol::Close(void)
{
	keep_running = false;
	if ( m_Future.valid() )
	{
		m_Future.get();
	}
	m_Socket4.Close();
	m_Socket6.Close();
}

////////////////////////////////////////////////////////////////////////////////////////
// streams helpers

void CProtocol::OnPacketIn(std::unique_ptr<CPacket> &packet, const CIp &ip)
{
	// find the stream
	auto stream = GetStream(packet->GetStreamId(), ip);
	if ( stream )
	{
		auto islast = packet->IsLastPacket(); // we'll need this after the std::move()!

		// and push the packet
		stream->Lock();
		stream->Push(std::move(packet));
		stream->Unlock();

		if (islast)
			g_Reflector.CloseStream(stream);
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// stream handle helpers

std::shared_ptr<CPacketStream> CProtocol::GetStream(uint16_t uiStreamId, const CIp &Ip)
{
	for ( auto it=m_Streams.begin(); it!=m_Streams.end(); it++ )
	{
		if ( (*it)->GetPacketStreamId() == uiStreamId )
		{
			// if Ip not nullptr, also check if IP match
			if ( (*it)->GetOwnerIp() != nullptr )
			{
				if ( Ip == *((*it)->GetOwnerIp()) )
				{
					return *it;
				}
			}
		}
	}
	// done
	return nullptr;
}

void CProtocol::CheckStreamsTimeout(void)
{
	for ( auto it=m_Streams.begin(); it!=m_Streams.end(); )
	{
		// time out ?
		(*it)->Lock();
		if ( (*it)->IsExpired() )
		{
			// yes, close it
			(*it)->Unlock();
			g_Reflector.CloseStream(*it);
			// and remove it
			it = m_Streams.erase(it);
		}
		else
		{
			(*it++)->Unlock();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// syntax helper

bool CProtocol::IsNumber(char c) const
{
	return ((c >= '0') && (c <= '9'));
}

bool CProtocol::IsLetter(char c) const
{
	return ((c >= 'A') && (c <= 'Z'));
}

bool CProtocol::IsSpace(char c) const
{
	return (c == ' ');
}

////////////////////////////////////////////////////////////////////////////////////////
// Receivers

ssize_t CProtocol::Receive6(uint8_t *buf, CIp &ip, int time_ms)
{
	return m_Socket6.Receive(buf, ip, time_ms);
}

ssize_t CProtocol::Receive4(uint8_t *buf, CIp &ip, int time_ms)
{
	return m_Socket4.Receive(buf, ip, time_ms);
}

ssize_t CProtocol::ReceiveDS(uint8_t *buf, CIp &ip, int time_ms)
{
	auto fd4 = m_Socket4.GetSocket();
	auto fd6 = m_Socket6.GetSocket();

	if (fd4 < 0)
	{
		if (fd6 < 0)
			return false;
		return m_Socket6.Receive(buf, ip, time_ms);
	}
	else if (fd6 < 0)
		return m_Socket4.Receive(buf, ip, time_ms);

	fd_set fset;
	FD_ZERO(&fset);
	FD_SET(fd4, &fset);
	FD_SET(fd6, &fset);
	int max = (fd4 > fd6) ? fd4 : fd6;
	struct timeval tv;
	tv.tv_sec = time_ms / 1000;
	tv.tv_usec = (time_ms % 1000) * 1000;

	auto rval = select(max+1, &fset, 0, 0, &tv);
	if (rval <= 0)
	{
		if (rval < 0)
			std::cerr << "ReceiveDS select error: " << strerror(errno) << std::endl;
		return 0;
	}

	if (FD_ISSET(fd4, &fset))
		return m_Socket4.ReceiveFrom(buf, ip);
	else
		return m_Socket6.ReceiveFrom(buf, ip);
}

////////////////////////////////////////////////////////////////////////////////////////
// dual stack senders

void CProtocol::Send(const char *buf, const CIp &Ip) const
{
	switch (Ip.GetFamily())
	{
	case AF_INET:
		m_Socket4.Send(buf, Ip);
		break;
	case AF_INET6:
		m_Socket6.Send(buf, Ip);
		break;
	default:
		std::cerr << "ERROR: wrong family: " << Ip.GetFamily() << std::endl;
		break;
	}
}

void CProtocol::Send(const uint8_t *buf, size_t size, const CIp &Ip) const
{
	switch (Ip.GetFamily())
	{
	case AF_INET:
		m_Socket4.Send(buf, size, Ip);
		break;
	case AF_INET6:
		m_Socket6.Send(buf, size, Ip);
		break;
	default:
		std::cerr << "ERROR: wrong family: " << Ip.GetFamily() << std::endl;
		break;
	}
}

void CProtocol::Send(const char *buf, const CIp &Ip, uint16_t port) const
{
	switch (Ip.GetFamily())
	{
	case AF_INET:
		m_Socket4.Send(buf, Ip, port);
		break;
	case AF_INET6:
		m_Socket6.Send(buf, Ip, port);
		break;
	default:
		std::cerr << "ERROR: wrong family: " << Ip.GetFamily() << " on port " << port << std::endl;
		break;
	}
}

void CProtocol::Send(const uint8_t *buf, size_t size, const CIp &Ip, uint16_t port) const
{
	switch (Ip.GetFamily())
	{
	case AF_INET:
		m_Socket4.Send(buf, size, Ip, port);
		break;
	case AF_INET6:
		m_Socket6.Send(buf, size, Ip, port);
		break;
	default:
		std::cerr << "ERROR: wrong family: " << Ip.GetFamily() << " on port " << port << std::endl;
		break;
	}
}
