//
//  cclients.cpp
//  m17ref
//
//  Created by Jean-Luc Deltombe (LX3JL) on 31/10/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020 Thomas A. Early, N7TAE
//
// ----------------------------------------------------------------------------
//    This file is part of m17ref.
//
//    m17ref is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    m17ref is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    with this software.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include "main.h"
#include "reflector.h"
#include "clients.h"


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

void CClients::AddClient(std::shared_ptr<CClient> client)
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
	// notify
	g_Reflector.OnClientsChanged();
}

void CClients::RemoveClient(std::shared_ptr<CClient> client)
{
	// look for the client
	bool found = false;
	for ( auto it=begin(); it!=end(); it++ )
	{
		// compare objetc pointers
		if ( *it == client )
		{
			// found it !
			if ( !(*it)->IsTransmitting() )
			{
				// remove it
				std::cout << "Client " << (*it)->GetCallsign() << " at " << (*it)->GetIp() << " removed with protocol " << (*it)->GetProtocolName();
				if ( (*it)->GetReflectorModule() != ' ' )
				{
					std::cout << " on module " << (*it)->GetReflectorModule();
				}
				std::cout << std::endl;
				m_Clients.erase(it);
				// notify
				g_Reflector.OnClientsChanged();
				break;
			}
		}
	}
}

bool CClients::IsClient(std::shared_ptr<CClient> client) const
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

std::shared_ptr<CClient> CClients::FindClient(const CIp &Ip)
{
	// find client
	for ( auto it=begin(); it!=end(); it++ )
	{
#ifdef MCLIENTS
		if ( ((*it)->GetIp() == Ip) && ((*it)->GetIp().GetPort() == Ip.GetPort()) )
#else
		if ((*it)->GetIp() == Ip)
#endif
		{
			return *it;
		}
	}

	// done
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////
// iterate on clients

std::shared_ptr<CClient> CClients::FindNextClient(std::list<std::shared_ptr<CClient>>::iterator &it)
{
	if ( it != end() )
		return *it++;

	return nullptr;
}

std::shared_ptr<CClient> CClients::FindNextClient(const CCallsign &Callsign, const CIp &Ip, std::list<std::shared_ptr<CClient>>::iterator &it)
{
	while ( it != end() )
	{
#ifdef MCLIENTS
		if ( ((*it)->GetIp() == Ip) && ((*it)->GetIp().GetPort() == Ip.GetPort()) && (*it)->GetCallsign().HasSameCallsign(Callsign) )
#else
		if ( ((*it)->GetIp() == Ip) && (*it)->GetCallsign().HasSameCallsign(Callsign) )
#endif
		{
			return *it++;
		}
		it++;
	}
	return nullptr;
}
