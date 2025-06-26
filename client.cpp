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
	m_Sock.Send(pack.GetCData(), pack.GetSize(), m_Ip);
}

////////////////////////////////////////////////////////////////////////////////////////
// reporting

void CClient::WriteXml(std::ofstream &xmlFile) const
{
	xmlFile << "<NODE>" << std::endl;
	xmlFile << "\t<CALLSIGN>" << m_Callsign << "</CALLSIGN>" << std::endl;
	xmlFile << "\t<IP>" << m_Ip.GetAddress() << "</IP>" << std::endl;
	xmlFile << "\t<LINKEDMODULE>" << m_ReflectorModule << "</LINKEDMODULE>" << std::endl;
	xmlFile << "\t<PROTOCOL>" << GetProtocolName() << "</PROTOCOL>" << std::endl;
	xmlFile << "\t<LISTENONLY>" << ((IsListenOnly()) ? "true" : "false") << "</LISTENONLY>" << std::endl;
	char mbstr[100];
	if (std::strftime(mbstr, sizeof(mbstr), "%FT%TZ", std::gmtime(&m_ConnectTime)))
	{
		xmlFile << "\t<CONNECTTIME>" << mbstr << "</CONNECTTIME>" << std::endl;
	}
	if (std::strftime(mbstr, sizeof(mbstr), "%FT%TZ", std::gmtime(&m_LastHeardTime)))
	{
		xmlFile << "\t<LASTHEARDTIME>" << mbstr << "</LASTHEARDTIME>" << std::endl;
	}
	xmlFile << "</NODE>" << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////////
// status

bool CClient::IsAlive(void) const
{
	return (m_LastKeepaliveTime.Time() < M17_KEEPALIVE_TIMEOUT);
}
