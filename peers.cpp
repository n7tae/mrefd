//
//  cpeers.cpp
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
#include "reflector.h"
#include "peers.h"


////////////////////////////////////////////////////////////////////////////////////////
// constructor


CPeers::CPeers() {}

////////////////////////////////////////////////////////////////////////////////////////
// destructors

CPeers::~CPeers()
{
	m_Mutex.lock();
	m_Peers.clear();
	m_Mutex.unlock();
}

////////////////////////////////////////////////////////////////////////////////////////
// manage peers

void CPeers::AddPeer(std::shared_ptr<CPeer> peer)
{
	// first check if peer already exists
	for ( auto it=begin(); it!=end(); it++ )
	{
		if (*peer == *(*it))
			// if found, just do nothing
			// so *peer keep pointing on a valid object
			// on function return
		{
			// delete new one
			return;
		}
	}

	// if not, append to the vector (put them in alphabetical order)
	auto pit = m_Peers.begin();
	for ( ; pit!=m_Peers.end(); pit++)
	{
		if (peer->GetCallsign().GetCS().compare((*pit)->GetCallsign().GetCS()) < 0) {
			m_Peers.insert(pit, peer);
			break;
		}
	}
	if (pit == m_Peers.end())
		m_Peers.push_back(peer);
	std::cout << "New peer " << peer->GetCallsign() << " at " << peer->GetIp() << " added with protocol " << peer->GetProtocolName()  << std::endl;
	// and append all peer's client to reflector client list
	// it is double lock safe to lock Clients list after Peers list
	CClients *clients = g_Reflector.GetClients();
	for ( auto cit=peer->cbegin(); cit!=peer->cend(); cit++ )
	{
		clients->AddClient(*cit);
	}
	g_Reflector.ReleaseClients();

	// notify
	g_Reflector.OnPeersChanged();
}

void CPeers::RemovePeer(std::shared_ptr<CPeer> peer)
{
	// look for the client
	for ( auto pit=begin(); pit!=end(); /*increment done in body */ )
	{
		// compare object pointers
		if (( *pit == peer ) && ( !(*pit)->IsTransmitting() ))
		{
			// remove all clients from reflector client list
			// it is double lock safe to lock Clients list after Peers list
			CClients *clients = g_Reflector.GetClients();
			for ( auto cit=peer->begin(); cit!=peer->end(); cit++ )
			{
				// this also delete the client object
				clients->RemoveClient(*cit);
			}
			// so clear it then
			(*pit)->ClearClients();
			g_Reflector.ReleaseClients();

			// remove it
			std::cout << "Peer " << (*pit)->GetCallsign() << " at " << (*pit)->GetIp() << " removed" << std::endl;
			pit = m_Peers.erase(pit);
			// notify
			g_Reflector.OnPeersChanged();
		}
		else
		{
			pit++;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// find peers

std::shared_ptr<CPeer> CPeers::FindPeer(const CIp &Ip)
{
	for ( auto it=begin(); it!=end(); it++ )
	{
		if ( ((*it)->GetIp() == Ip) )
		{
			return *it;
		}
	}

	return nullptr;
}

std::shared_ptr<CPeer> CPeers::FindPeer(const CCallsign &Callsign, const CIp &Ip)
{
	for ( auto it=begin(); it!=end(); it++ )
	{
		if ( (*it)->GetCallsign().HasSameCallsign(Callsign) && ((*it)->GetIp() == Ip) )
		{
			return *it;
		}
	}

	return nullptr;
}

std::shared_ptr<CPeer> CPeers::FindPeer(const CCallsign &Callsign)
{
	for ( auto it=begin(); it!=end(); it++ )
	{
		if ( (*it)->GetCallsign().HasSameCallsign(Callsign) )
		{
			return *it;
		}
	}

	return nullptr;
}


////////////////////////////////////////////////////////////////////////////////////////
// iterate on peers

std::shared_ptr<CPeer> CPeers::FindNextPeer(std::list<std::shared_ptr<CPeer>>::iterator &it)
{
	if ( it!=end() )
		return *it++;

	return nullptr;
}
