//
//  cpeer.cpp
//  M17Refd
//
//  Created by Jean-Luc Deltombe (LX3JL) on 10/12/2016.
//  Copyright © 2016 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020 Thomas A. Early, N7TAE
//
// ----------------------------------------------------------------------------
//    This file is part of M17Refd.
//
//    M17Refd is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    M17Refd is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    with this software.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include "main.h"
#include <string.h>
#include "reflector.h"
#include "peer.h"


////////////////////////////////////////////////////////////////////////////////////////
// constructor


CPeer::CPeer()
{
	::memset(m_ReflectorModules, 0, sizeof(m_ReflectorModules));
	m_ConnectTime = std::time(nullptr);
	m_LastHeardTime = std::time(nullptr);
}

CPeer::CPeer(const CCallsign &callsign, const CIp &ip, const char *modules)
{
	m_Callsign = callsign;
	m_Ip = ip;
	::memset(m_ReflectorModules, 0, sizeof(m_ReflectorModules));
	::strncpy(m_ReflectorModules, modules, sizeof(m_ReflectorModules)-1);
	m_LastKeepaliveTime.Now();
	m_ConnectTime = std::time(nullptr);
	m_LastHeardTime = std::time(nullptr);

	std::cout << "Adding M17 peer " << callsign << " module(s) " << modules << std::endl;

	// and construct the M17 clients
	for (auto p=modules; *p; p++)
	{
		// create and append to vector
		m_Clients.push_back(std::make_shared<CClient>(callsign, ip, *p));
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

bool CPeer::IsAMaster(void) const
{
	for ( auto it=cbegin(); it!=cend(); it++ )
	{
		if ((*it)->IsAMaster())
			return true;
	}
	return false;
}

void CPeer::Alive(void)
{
	m_LastKeepaliveTime.Now();
	for ( auto it=begin(); it!=end(); it++ )
	{
		(*it)->Alive();
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// reporting

void CPeer::WriteXml(std::ofstream &xmlFile)
{
	xmlFile << "<PEER>" << std::endl;
	xmlFile << "\t<Callsign>" << m_Callsign << "</Callsign>" << std::endl;
	xmlFile << "\t<IP>" << m_Ip.GetAddress() << "</IP>" << std::endl;
	xmlFile << "\t<LinkedModule>" << m_ReflectorModules << "</LinkedModule>" << std::endl;
	xmlFile << "\t<Protocol>" << GetProtocolName() << "</Protocol>" << std::endl;
	char mbstr[100];
	if (std::strftime(mbstr, sizeof(mbstr), "%A %c", std::localtime(&m_ConnectTime)))
	{
		xmlFile << "\t<ConnectTime>" << mbstr << "</ConnectTime>" << std::endl;
	}
	if (std::strftime(mbstr, sizeof(mbstr), "%A %c", std::localtime(&m_LastHeardTime)))
	{
		xmlFile << "\t<LastHeardTime>" << mbstr << "</LastHeardTime>" << std::endl;
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

int CPeer::GetProtocolRevision(const CVersion &version) const
{
	return version.GetMajor();
}
