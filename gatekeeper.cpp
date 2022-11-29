//
//  cgatekeeper.cpp
//  M17Refd
//
//  Created by Jean-Luc Deltombe (LX3JL) on 07/11/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//
//  Copyright © 2022 Thomas A. Early N7TAE. All rights reserved.
//
// ----------------------------------------------------------------------------
//    This file is part of mrefd.
//
//    mrefd is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    mrefd is distributed in the hope that it will be useful,
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
#include "ifile.h"

extern CReflector g_Reflector;
extern CConfigure g_CFG;
extern CIFileMap g_IFile;

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
	g_IFile.LoadFromFile(g_CFG.GetInterlinkPath().c_str());

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

bool CGateKeeper::MayLink(const CCallsign &callsign, const CIp &ip, char *modules) const
{
	bool ok = false;
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
	return IsNodeListedOk(callsign);
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
		if ( g_IFile.NeedReload() )
		{
			g_IFile.ReloadFromFile();
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
	m_NodeWhiteSet.Lock();
	if ( ! m_NodeWhiteSet.empty() )
	{
		ok = m_NodeWhiteSet.IsMatched(callsign.GetCS());
	}
	m_NodeWhiteSet.Unlock();

	// then check if not blacklisted
	m_NodeBlackSet.Lock();
	ok = ok && !m_NodeBlackSet.IsMatched(callsign.GetCS());
	m_NodeBlackSet.Unlock();

	// done
	return ok;

}

bool CGateKeeper::IsPeerListedOk(const CCallsign &callsign, const CIp &ip, const char *modules) const
{
	bool ok = false;

	// look for an exact match in the list
	g_IFile.Lock();
	if ( ! g_IFile.empty() )
	{
		// find an exact match
		ok = g_IFile.IsCallsignListed(callsign, ip, modules);
	}
	g_IFile.Unlock();

	// done
	return ok;
}
