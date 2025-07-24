//
//  cpeer.cpp
//  mrefd
//
//  Created by Jean-Luc Deltombe (LX3JL) on 10/12/2016.
//  Copyright © 2016 Jean-Luc Deltombe (LX3JL). All rights reserved.
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

#include <iostream>
#include <fstream>

#include <string.h>
#include "reflector.h"
#include "peer.h"

CPeer::CPeer(const CCallsign cs, const CIp ip, EClientType type, const std::string &mods, const std::string &url, const CUdpSocket &sock) : m_Callsign(cs), m_Ip(ip), m_refType(type), m_sharedModules(mods), m_dashUrl(url)
{
	m_LastKeepaliveTime.Start();
	m_ConnectTime = std::time(nullptr);

	std::cout << "Adding M17 peer " << cs << " module(s) " << mods << std::endl;
	CCallsign clientcs(cs);
	// and construct the M17 clients
	for (auto m : m_sharedModules)
	{
		clientcs.SetModule(m);
		// create and append to list
		m_Clients[m] = std::make_shared<CClient>(clientcs, ip, type, m, sock);
	}
}

CPeer::~CPeer()
{
	m_Clients.clear();
}

bool CPeer::IsTransmitting(void) const
{
	for (auto item : m_Clients)
	{
		if (item.second->IsTransmitting())
			return true;
	}
	return false;
}

void CPeer::Alive(void)
{
	m_LastKeepaliveTime.Start();
	for (auto item : m_Clients)
	{
		item.second->Alive();
	}
}

void CPeer::WriteXml(std::ofstream &xmlFile) const
{
	time_t lht = 0;
	for (auto &item : m_Clients)
	{
		auto t = item.second->GetLastHeardTime();
		if (t > lht)
			lht = t;
	}
	xmlFile << "<PEER>" << std::endl;
	xmlFile << "\t<CALLSIGN>" << m_Callsign << "</CALLSIGN>" << std::endl;
	xmlFile << "\t<IP>" << m_Ip.GetAddress() << "</IP>" << std::endl;
	xmlFile << "\t<LINKEDMODULE>" << m_sharedModules << "</LINKEDMODULE>" << std::endl;
	xmlFile << "\t<DASHBOARDURL>" << m_dashUrl << "</DASHBOARDURL>" << std::endl;
	xmlFile << "\t<PROTOCOL>" << GetProtocolName() << "</PROTOCOL>" << std::endl;
	char mbstr[100];
	if (std::strftime(mbstr, sizeof(mbstr), "%FT%TZ", std::gmtime(&m_ConnectTime)))
	{
		xmlFile << "\t<CONNECTTIME>" << mbstr << "</CONNECTTIME>" << std::endl;
	}
	if (std::strftime(mbstr, sizeof(mbstr), "%FT%TZ", std::gmtime(&lht)))
	{
		xmlFile << "\t<LASTHEARDTIME>" << mbstr << "</LASTHEARDTIME>" << std::endl;
	}
	xmlFile << "</PEER>" << std::endl;
}

bool CPeer::IsAlive(void) const
{
	for (auto item : m_Clients)
	{
		if (not item.second->IsAlive())
			return false;
	}
	return true;
}

SPClient CPeer::GetClient(char m)
{
	auto item = m_Clients.find(m);
	if (m_Clients.end() == item)
		return nullptr;
	else
		return item->second;
}
