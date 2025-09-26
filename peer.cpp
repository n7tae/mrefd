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

CPeer::CPeer(const CCallsign cs, const CIp ip, EClientType type, const std::string &mods, const CUdpSocket &sock) : m_Callsign(cs), m_Ip(ip), m_refType(type), m_sharedModules(mods)
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

void CPeer::AddPeerState(nlohmann::json &jdata) const
{
	time_t lht = 0;
	for (auto &item : m_Clients)
	{
		auto t = item.second->GetLastHeardTime();
		if (t > lht)
			lht = t;
	}
	nlohmann::json json;
	json["Callsign"] = m_Callsign.GetCS();
	json["IP"] = m_Ip.GetAddress();
	json["Modules"] = m_sharedModules;
	json["Protocol"] = GetProtocolName();
	json["ConnectTime"] = m_ConnectTime;
	json["LastHeardTime"] = lht;
	jdata.emplace_back(json);
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
