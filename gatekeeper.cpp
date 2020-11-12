//
//  cgatekeeper.cpp
//  M17Refd
//
//  Created by Jean-Luc Deltombe (LX3JL) on 07/11/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
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
#include "timepoint.h"
#include "reflector.h"
#include "gatekeeper.h"

////////////////////////////////////////////////////////////////////////////////////////

CGateKeeper g_GateKeeper;


////////////////////////////////////////////////////////////////////////////////////////
// constructor

CGateKeeper::CGateKeeper()
{
	keep_running = true;
}

////////////////////////////////////////////////////////////////////////////////////////
// destructor

CGateKeeper::~CGateKeeper()
{
	Close();
}


////////////////////////////////////////////////////////////////////////////////////////
// init & clode

bool CGateKeeper::Init(void)
{

	// load lists from files
	m_NodeWhiteSet.LoadFromFile(WHITELIST_PATH);
	m_NodeBlackSet.LoadFromFile(BLACKLIST_PATH);
	m_PeerMap.LoadFromFile(INTERLINKLIST_PATH);

	// reset run flag
	keep_running = true;

	// start  thread;
	m_Future = std::async(std::launch::async, &CGateKeeper::Thread, this);

	return true;
}

void CGateKeeper::Close(void)
{
	// kill threads
	keep_running = false;
	if ( m_Future.valid() )
	{
		m_Future.get();
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// authorizations

bool CGateKeeper::MayLink(const CCallsign &callsign, const CIp &ip, char *modules) const
{
	bool ok;
	if (callsign.GetCS(4).compare("M17-"))
	{
		auto clients = g_Reflector.GetClients();
		if (clients->FindClient(ip))
			ok = false;
		else
			ok = IsNodeListedOk(callsign);
		g_Reflector.ReleaseClients();
	}
	else
	{
		ok = IsPeerListedOk(callsign, ip, modules);
	}

	if ( !ok )
	{
		std::cout << "Gatekeeper blocking linking of " << callsign << " @ " << ip << std::endl;
	}

	// done
	return ok;
}

bool CGateKeeper::MayTransmit(const CCallsign &callsign, const CIp &ip) const
{
	bool ok = IsNodeListedOk(callsign);

	if ( !ok )
	{
		std::cout << "Gatekeeper blocking transmitting of " << callsign << " @ " << ip << std::endl;
	}

	// done
	return ok;
}

////////////////////////////////////////////////////////////////////////////////////////
// thread

void CGateKeeper::Thread()
{
	while (keep_running)
	{
		// Wait 30 seconds
		for (int i=0; i<15 && keep_running; i++)
			CTimePoint::TaskSleepFor(2000);

		// have lists files changed ?
		if ( m_NodeWhiteSet.NeedReload() )
		{
			m_NodeWhiteSet.ReloadFromFile();
		}
		if ( m_NodeBlackSet.NeedReload() )
		{
			m_NodeBlackSet.ReloadFromFile();
		}
		if ( m_PeerMap.NeedReload() )
		{
			m_PeerMap.ReloadFromFile();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// operation helpers

bool CGateKeeper::IsNodeListedOk(const CCallsign &callsign) const
{
	bool ok = true;

	// next, check callsign
	// first check if callsign is in white list
	// note if white list is empty, everybody is authorized
	const_cast<CBWSet &>(m_NodeWhiteSet).Lock();
	if ( !m_NodeWhiteSet.empty() )
	{
		ok = m_NodeWhiteSet.IsMatched(callsign.GetCS());
	}
	const_cast<CBWSet &>(m_NodeWhiteSet).Unlock();

	// then check if not blacklisted
	const_cast<CBWSet &>(m_NodeBlackSet).Lock();
	ok = ok && !m_NodeBlackSet.IsMatched(callsign.GetCS());
	const_cast<CBWSet &>(m_NodeBlackSet).Unlock();

	// done
	return ok;

}

bool CGateKeeper::IsPeerListedOk(const CCallsign &callsign, const CIp &ip, const char *modules) const
{
	bool ok;

	// look for an exact match in the list
	const_cast<CPeerMap &>(m_PeerMap).Lock();
	if ( !m_PeerMap.empty() )
	{
		// find an exact match
		ok = m_PeerMap.IsCallsignListed(callsign, ip, modules);
	}
	const_cast<CPeerMap &>(m_PeerMap).Unlock();

	// done
	return ok;
}
