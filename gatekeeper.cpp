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
	m_NodeWhiteList.LoadFromFile(WHITELIST_PATH);
	m_NodeBlackList.LoadFromFile(BLACKLIST_PATH);
	m_PeerList.LoadFromFile(INTERLINKLIST_PATH);

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

bool CGateKeeper::MayLink(const CCallsign &callsign, const CIp &ip, int protocol, char *modules) const
{
	bool ok;
	if (callsign.GetCS(4).compare("M17-"))
	{
		ok = IsNodeListedOk(callsign, ip);
	}
	else
	{
		ok = IsPeerListedOk(callsign, ip, modules);
	}

	if ( !ok )
	{
		std::cout << "Gatekeeper blocking linking of " << callsign << " @ " << ip << " using protocol " << protocol << std::endl;
	}

	// done
	return ok;
}

bool CGateKeeper::MayTransmit(const CCallsign &callsign, const CIp &ip, int protocol, char module) const
{
	bool ok = IsNodeListedOk(callsign, ip, module);

	if ( !ok )
	{
		std::cout << "Gatekeeper blocking transmitting of " << callsign << " @ " << ip << " using protocol " << protocol << std::endl;
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
		if ( m_NodeWhiteList.NeedReload() )
		{
			m_NodeWhiteList.ReloadFromFile();
		}
		if ( m_NodeBlackList.NeedReload() )
		{
			m_NodeBlackList.ReloadFromFile();
		}
		if ( m_PeerList.NeedReload() )
		{
			m_PeerList.ReloadFromFile();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// operation helpers

bool CGateKeeper::IsNodeListedOk(const CCallsign &callsign, const CIp &ip, char module) const
{
	bool ok = true;

	// first check IP

	// next, check callsign
	if ( ok )
	{
		// first check if callsign is in white list
		// note if white list is empty, everybody is authorized
		const_cast<CCallsignList &>(m_NodeWhiteList).Lock();
		if ( !m_NodeWhiteList.empty() )
		{
			ok = m_NodeWhiteList.IsCallsignListedWithWildcard(callsign, module);
		}
		const_cast<CCallsignList &>(m_NodeWhiteList).Unlock();

		// then check if not blacklisted
		const_cast<CCallsignList &>(m_NodeBlackList).Lock();
		ok &= !m_NodeBlackList.IsCallsignListedWithWildcard(callsign);
		const_cast<CCallsignList &>(m_NodeBlackList).Unlock();
	}

	// done
	return ok;

}

bool CGateKeeper::IsPeerListedOk(const CCallsign &callsign, const CIp &ip, const char *modules) const
{
	bool ok = true;

	if ( ok )
	{
		// look for an exact match in the list
		const_cast<CPeerCallsignList &>(m_PeerList).Lock();
		if ( !m_PeerList.empty() )
		{
			// find an exact match
			ok = m_PeerList.IsCallsignListed(callsign, ip, modules);
		}
		const_cast<CPeerCallsignList &>(m_PeerList).Unlock();
	}

	// done
	return ok;
}
