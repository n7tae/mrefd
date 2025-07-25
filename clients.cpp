//
//  cclients.cpp
//  mrefd
//
//  Created by Jean-Luc Deltombe (LX3JL) on 31/10/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020,2022 Thomas A. Early, N7TAE
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

#include "reflector.h"
#include "clients.h"
#include "configure.h"

extern CReflector g_Reflector;
extern CConfigure g_CFG;

////////////////////////////////////////////////////////////////////////////////////////
// constructor


CClients::CClients()
{
}

////////////////////////////////////////////////////////////////////////////////////////
// destructors

CClients::~CClients()
{
	m_Mutex.lock();
	m_Clients.clear();
	m_Mutex.unlock();
}

////////////////////////////////////////////////////////////////////////////////////////
// manage Clients

void CClients::AddClient(SPClient client)
{
	// first check if client already exists
	for ( auto it=begin(); it!=end(); it++ )
	{
		if (*client == *(*it))
			// if found, just do nothing
			// so *client keep pointing on a valid object
			// on function return
		{
			// delete new one
			return;
		}
	}

	// and append
	m_Clients.push_back(client);
	std::cout << "New client " << client->GetCallsign() << " at " << client->GetIp() << " added with protocol " << client->GetProtocolName();
	if ( client->GetReflectorModule() != ' ' )
	{
		std::cout << " on module " << client->GetReflectorModule();
	}
	std::cout << std::endl;
}

void CClients::RemoveClient(SPClient client)
{
	if (not client)
		return;
	for (auto it=m_Clients.begin(); it!=m_Clients.end(); it++)
	{
		const auto c = *it;
		if (c == client)
		{
			// found it !
			std::cout << "Client " << c->GetCallsign() << " on module " << c->GetReflectorModule() << " at " << c->GetIp() << " using protocol " << c->GetProtocolName() << " removed" << std::endl;
			m_Clients.erase(it);
			break;
		}
	}
}

bool CClients::IsClient(SPClient client) const
{
	for ( auto it=cbegin(); it!=cend(); it++ )
	{
		if (*it == client)
			return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////
// find Clients

SPClient CClients::FindClient(const CIp &Ip)
{
	// find client
	for ( auto it=begin(); it!=end(); it++ )
	{
		if (((*it)->GetIp() == Ip) && ((*it)->GetIp().GetPort() == Ip.GetPort()))
			return *it;
	}
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////
// iterate on clients

SPClient CClients::FindNextClient(std::list<SPClient>::iterator &it)
{
	if ( it != end() )
		return *it++;

	return nullptr;
}

SPClient CClients::FindNextClient(const char mod, std::list<SPClient>::iterator &it)
{
	while ( it != end() )
	{
		if (mod == (*it)->GetReflectorModule())
			return *it++;
		it++;
	}
	return nullptr;
}

SPClient CClients::FindNextClient(const CCallsign &Callsign, const CIp &Ip, std::list<SPClient>::iterator &it)
{
	while ( it != end() )
	{
		if (((*it)->GetIp() == Ip) and (*it)->GetCallsign().HasSameCallsign(Callsign) and ((*it)->GetIp().GetPort() == Ip.GetPort()))
			return *it++;
		it++;
	}
	return nullptr;
}
