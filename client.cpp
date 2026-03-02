//
//  cclient.cpp
//  mrefd
//
//  Created by Jean-Luc Deltombe (LX3JL) on 31/10/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020,2022 Thomas A. Early N7TAE
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

#include <ctime>
#include <iostream>
#include <fstream>
#include <string.h>

#include "defines.h"
#include "client.h"
#include "configure.h"

extern CConfigure g_CFG;

////////////////////////////////////////////////////////////////////////////////////////
// constructors

CClient::CClient(const CCallsign cs, const CIp ip, EClientType type, char mod, const CUdpSocket &sock) : m_Callsign(cs), m_Ip(ip), m_Type(type), m_ReflectorModule(mod), m_Sock(sock)
{
	m_isTXing = false;
	m_LastKeepaliveTime.Start();
	m_ConnectTime = std::time(nullptr);
	m_LastHeardTime = std::time(nullptr);
}


////////////////////////////////////////////////////////////////////////////////////////
// status

void CClient::Alive(void)
{
	m_LastKeepaliveTime.Start();
}


////////////////////////////////////////////////////////////////////////////////////////
// operators

bool CClient::operator ==(const CClient &client) const
{
	return (client.m_Callsign == m_Callsign) and
	       (client.m_Ip == m_Ip) and
	       (client.m_ReflectorModule == m_ReflectorModule) and
		   (client.m_Ip.GetPort() == m_Ip.GetPort());
}

////////////////////////////////////////////////////////////////////////////////////////
// function

void CClient::SendPacket(const CPacket &pack) const
{
	auto size = pack.GetSize();
	m_Sock.Send(pack.GetCData(), size, m_Ip);
}

////////////////////////////////////////////////////////////////////////////////////////
// reporting

void CClient::AddClient(nlohmann::json &data) const
{
	std::string m;
	m.append(1, m_ReflectorModule);
	data += {
		{ "Callsign",      m_Callsign.GetCS() },
		{ "IP",            m_Ip.GetAddress()  },
		{ "Module",        m                  },
		{ "Protocol",      GetProtocolName()  },
		{ "ListenOnly",    IsListenOnly()     },
		{ "ConnectTime",   m_ConnectTime      },
		{ "LastHeardTime", m_LastHeardTime    }
	};
}

////////////////////////////////////////////////////////////////////////////////////////
// status

bool CClient::IsAlive(void) const
{
	return (m_LastKeepaliveTime.Time() < M17_KEEPALIVE_TIMEOUT);
}
