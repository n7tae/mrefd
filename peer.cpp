//
//  cpeer.cpp
//  mrefd
//
//  Created by Jean-Luc Deltombe (LX3JL) on 10/12/2016.
//  Copyright © 2016 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020 Thomas A. Early, N7TAE
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

////////////////////////////////////////////////////////////////////////////////////////
// constructor


CPeer::CPeer()
{
	m_ConnectTime = std::time(nullptr);
}

CPeer::CPeer(const CCallsign &callsign, const CIp &ip, const char *modules) : CPeer()
{
	m_Callsign = callsign;
	m_Ip = ip;
	m_ReflectorModules.assign(modules);
	m_LastKeepaliveTime.Start();

	std::cout << "Adding M17 peer " << callsign << " module(s) " << modules << std::endl;

	// and construct the M17 clients
	for (const auto mod : m_ReflectorModules)
	{
		CCallsign clientcs(callsign);
		clientcs.SetModule(mod);
		// create and append to list
		m_Clients.push_back(std::make_shared<CClient>(clientcs, ip, mod));
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// destructors

CPeer::~CPeer()
{
	m_Clients.clear();
}

////////////////////////////////////////////////////////////////////////////////////////
// operators

bool CPeer::operator ==(const CPeer &peer) const
{
	if (peer.m_Callsign != m_Callsign)
		return false;
	if (peer.m_Ip != m_Ip)
		return false;
	auto it1 = cbegin();
	auto it2 = peer.cbegin();
	while (true)
	{
		if (it1==cend() && it2==peer.cend())
			break;
		if (it1==cend() || it2==peer.cend())
			return false;
		if (*it1 != *it2)
			return false;
		it1++;
		it2++;
	}
	return true;
}


////////////////////////////////////////////////////////////////////////////////////////
// status

bool CPeer::IsTransmitting(void) const
{
	for ( auto it=cbegin(); it!=cend(); it++ )
	{
		if ((*it)->IsTransmitting())
			return true;
	}
	return false;
}

void CPeer::Alive(void)
{
	m_LastKeepaliveTime.Start();
	for ( auto it=begin(); it!=end(); it++ )
	{
		(*it)->Alive();
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// reporting

void CPeer::WriteXml(std::ofstream &xmlFile)
{
	time_t lht = 0;
	for (auto &client : m_Clients)
	{
		auto t = client->GetLastHeardTime();
		if (t > lht)
			lht = t;
	}
	xmlFile << "<PEER>" << std::endl;
	xmlFile << "\t<CALLSIGN>" << m_Callsign << "</CALLSIGN>" << std::endl;
	xmlFile << "\t<IP>" << m_Ip.GetAddress() << "</IP>" << std::endl;
	xmlFile << "\t<LINKEDMODULE>" << m_ReflectorModules << "</LINKEDMODULE>" << std::endl;
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

////////////////////////////////////////////////////////////////////////////////////////
// status

bool CPeer::IsAlive(void) const
{
	for ( auto it=cbegin(); it!=cend(); it++ )
	{
		if (! (*it)->IsAlive())
			return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////
// revision helper

uint8_t CPeer::GetProtocolRevision(const CVersion &version) const
{
	return version.GetMajor();
}
