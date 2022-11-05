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

#include "main.h"
#include "timer.h"
#include "reflector.h"
#include "gatekeeper.h"
#include "configure.h"
#include "ifile.h"

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
#ifndef NO_DHT
	// start the dht instance
	refID = dht::crypto::generateIdentity(g_CFG.GetCallsign());
	privateKey = dht::crypto::PrivateKey::generate();
	node.run(17171, refID, true);
#endif

#ifdef USE_SAVED_DHT_STATE
	// bootstrap the DHT from either saved nodes from a previous run,
	// or from the configured node
	std::string path(BOOTFILE);
	// Try to import nodes from binary file
	std::ifstream myfile(path, std::ios::binary|std::ios::ate);
	if (myfile.is_open())
	{
		msgpack::unpacker pac;
		auto size = myfile.tellg();
		myfile.seekg (0, std::ios::beg);
		pac.reserve_buffer(size);
		myfile.read (pac.buffer(), size);
		pac.buffer_consumed(size);
		// Import nodes
		msgpack::object_handle oh;
		while (pac.next(oh)) {
			auto imported_nodes = oh.get().as<std::vector<dht::NodeExport>>();
			std::cout << "Importing " << imported_nodes.size() << " nodes" << std::endl;
			node.bootstrap(imported_nodes);
		}
		myfile.close();
	}
	else
#endif
#ifndef NO_DHT
	{
		node.bootstrap(g_CFG.GetBootstrap(), "17171");
	}
#endif

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
#ifndef NO_DHT
	// kill the DHT
	node.shutdown({}, true);
	node.join();
#endif

	// kill threads
	keep_running = false;
	if ( m_Future.valid() )
	{
		m_Future.get();
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// Publish DHT

#ifndef NO_DHT
void CGateKeeper::PutDHTInfo()
{
	const std::string cs(g_CFG.GetCallsign());
	SReflectorData1 rd;
	rd.cs.assign(cs);
	rd.ipv4.assign(g_CFG.GetIPv4ExtAddr());
	rd.ipv6.assign(g_CFG.GetIPv6ExtAddr());
	rd.mods.assign(g_CFG.GetModules());
	rd.encryptmods.assign(g_CFG.GetEncryptedMods());
	rd.url.assign(g_CFG.GetURL());
	rd.email.assign(g_CFG.GetEmailAddr());
	rd.port = (unsigned short)g_CFG.GetPort();

	auto peers = g_Reflector.GetPeers();
	for (auto pit=peers->cbegin(); pit!=peers->cend(); pit++)
	{
		const auto modules((*pit)->GetReflectorModules());
		rd.peers.emplace_back(std::pair<std::string,std::string>((*pit)->GetCallsign().GetCS(), modules));
	}
	g_Reflector.ReleasePeers();

	auto nv = std::make_shared<dht::Value>(rd);
	Dump("My dht::Value =", nv->data.data(), nv->data.size());
	nv->user_type.assign("reflector-mrefd-1");
	nv->sign(privateKey);

	if (! node.isRunning())
	{
		std::cout << "Waiting for node" << std::flush;

		unsigned count = 30u;
		for (; count > 0 && not node.isRunning(); count--)
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
			std::cout << '.' << std::flush;
		}
		if (count)
			std::cout << "done" << std::endl;
		else
			std::cout << "Error waiting!" << std::endl;
	}

	node.putSigned(
		dht::InfoHash::get(cs),
		nv,
		[](bool success){ std::cout << "PutDHTInfo() " << (success ? "successful" : "unsuccessful") << std::endl; },
		true
	);
}
#endif

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
