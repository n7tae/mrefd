//
//  cgatekeeper.cpp
//  mrefd
//
//  Created by Jean-Luc Deltombe (LX3JL) on 07/11/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//
//  Copyright © 2022 Thomas A. Early N7TAE. All rights reserved.
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

#include <thread>
#include "timer.h"
#include "reflector.h"
#include "gatekeeper.h"
#include "configure.h"
#include "interlinks.h"

extern CReflector g_Reflector;
extern CConfigure g_CFG;
extern CInterlinks g_Interlinks;

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
	m_NodeWhiteSet.LoadFromFile(g_CFG.GetWhitePath().c_str());
	m_NodeBlackSet.LoadFromFile(g_CFG.GetBlackPath().c_str());
	g_Interlinks.LoadFromFile(g_CFG.GetInterlinkPath().c_str());

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
// Publish DHT

////////////////////////////////////////////////////////////////////////////////////////
// authorizations

bool CGateKeeper::ClientMayLink(const CCallsign &cs, const CIp &ip) const
{
	bool ok = false;

	auto clients = g_Reflector.GetClients();
	if (clients->FindClient(ip))
	{
		std::cout << "GateKeeper: " << cs << " is trying to link again" << std::endl;
		ok = false;	// already linked!
	}
	else
		ok = IsNodeListed(cs);
	g_Reflector.ReleaseClients();

	return ok;
}

bool CGateKeeper::MayTransmit(const CCallsign &callsign, const CIp &/*ip*/) const
{
	return IsNodeListed(callsign);
}

////////////////////////////////////////////////////////////////////////////////////////
// thread

void CGateKeeper::Thread()
{
	while (keep_running)
	{
		// Wait 30 seconds
		for (int i=0; i<15 && keep_running; i++)
			std::this_thread::sleep_for(std::chrono::microseconds(2000));

		// have lists files changed ?
		if ( m_NodeWhiteSet.NeedReload() )
		{
			m_NodeWhiteSet.ReloadFromFile();
		}
		if ( m_NodeBlackSet.NeedReload() )
		{
			m_NodeBlackSet.ReloadFromFile();
		}
		g_Interlinks.Lock();
		if ( g_Interlinks.NeedReload() )
		{
			g_Interlinks.ReloadFromFile();
		}
		g_Interlinks.Unlock();
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// operation helpers

bool CGateKeeper::IsNodeListed(const CCallsign &cs) const
{
	// next, check callsign
	// first check if callsign is in white list
	// note if white list is empty, everybody is authorized
	if (not m_NodeWhiteSet.IsEmptyOrMatched(cs.GetCS()))
	{
		std::cout << cs.GetCS() << " has been blocked by the whitelist" << std::endl;
		return false;
	}


	// then check if not blacklisted
	if (not m_NodeBlackSet.IsMatched(cs.GetCS()))
	{
		std::cout << cs.GetCS() << " has been blocked by the blacklist" << std::endl;
		return false;
	}

	// done
	return true;
}

bool CGateKeeper::PeerMayLink(const CCallsign &cs) const
{
	// look for an exact match in the list
	g_Interlinks.Lock();
	if ( ! g_Interlinks.empty() )
	{
		// find an exact match
		if (g_Interlinks.Find(cs.GetCS()))
			return true;
	}
	g_Interlinks.Unlock();

	std::cout << cs << " was not found in the interlink file" << std::endl;
	// done
	return false;
}
