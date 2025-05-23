//
//  creflector.cpp
//  mrefd
//
//  Created by Jean-Luc Deltombe (LX3JL) on 31/10/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020,2022-2025 Thomas A. Early, N7TAE
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
#include <iostream>
#include <fstream>
#include <string.h>

#include "defines.h"
#include "reflector.h"
#include "gatekeeper.h"
#include "configure.h"
#include "version.h"
#include "ifile.h"

CReflector g_Reflector;
extern CGateKeeper g_GateKeeper;
extern CConfigure g_CFG;
extern CVersion g_Version;
extern CIFileMap g_IFile;

////////////////////////////////////////////////////////////////////////////////////////
// constructor

CReflector::CReflector()
{
#ifndef NO_DHT
	peers_put_count = 0;
#endif
	keep_running = true;
}

////////////////////////////////////////////////////////////////////////////////////////
// destructor

CReflector::~CReflector()
{
	keep_running = false;
	if ( m_XmlReportFuture.valid() )
	{
		m_XmlReportFuture.get();
	}
}


////////////////////////////////////////////////////////////////////////////////////////
// operation

bool CReflector::Start(const char *cfgfilename)
{
	if (g_CFG.ReadData(cfgfilename))
		return true;
	// let's go!
	keep_running = true;

	// init gate keeper. It can only return true!
	g_GateKeeper.Init();

#ifndef NO_DHT
	// start the dht instance
	refhash = dht::InfoHash::get(g_CFG.GetCallsign());
	node.run(17171, dht::crypto::generateIdentity(g_CFG.GetCallsign()), true, 59973);
	std::ifstream myfile;
	const auto path = g_CFG.GetDHTSavePath();
	if (path.size() > 0)
		myfile.open(path, std::ios::binary|std::ios::ate);
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
			std::cout << "Importing " << imported_nodes.size() << " ham-dht nodes from " << path << std::endl;
			node.bootstrap(imported_nodes);
		}
		myfile.close();
	}
	else
	{
		const auto bsnode = g_CFG.GetBootstrap();
		if (bsnode.size())
		{
			std::cout << "Bootstrapping from " << bsnode << std::endl;
			node.bootstrap(bsnode, "17171");
		}
		else
		{
			std::cout << "WARNING: The DHT is not bootstrapping from any node!" << std::endl;
		}
	}
#endif

	// create protocols
	if (! m_Protocol.Initialize(g_CFG.GetPort(), g_CFG.GetIPv4BindAddr(), g_CFG.GetIPv6BindAddr()))
	{
		m_Protocol.Close();
		return true;
	}

	// start the reporting threads
	m_XmlReportFuture = std::async(std::launch::async, &CReflector::XmlReportThread, this);
#ifndef NO_DHT
	PutDHTConfig();
#endif

	return false;
}

void CReflector::Stop(void)
{
	// stop & delete all threads
	keep_running = false;

	// stop & delete report threads
	if ( m_XmlReportFuture.valid() )
	{
		m_XmlReportFuture.get();
	}

	// close protocols
	m_Protocol.Close();

	// close gatekeeper
	g_GateKeeper.Close();

#ifndef NO_DHT
	// save the state of the DHT network
	const auto path = g_CFG.GetDHTSavePath();
	if (path.size() > 0)
	{
		auto exnodes = node.exportNodes();
		if (exnodes.size())
		{
			// Export nodes to binary file
			std::ofstream myfile(path, std::ios::binary | std::ios::trunc);
			if (myfile.is_open())
			{
				std::cout << "Saving " << exnodes.size() << " nodes to " << path << std::endl;
				msgpack::pack(myfile, exnodes);
				myfile.close();
			}
			else
				std::cerr << "Trouble opening " << path << std::endl;
		}
		else
			std::cout << "There are no DHT network nodes to save!" << std::endl;
	}

	// kill the DHT
	node.cancelPut(refhash, toUType(EMrefdValueID::Config));
	node.cancelPut(refhash, toUType(EMrefdValueID::Peers));
	node.shutdown({}, true);
	node.join();
#endif
}

////////////////////////////////////////////////////////////////////////////////////////
// report threads

void CReflector::XmlReportThread()
{
	while (keep_running)
	{
		const std::string xmlfilepath(g_CFG.GetXmlPath());
		// report to xml file
		std::ofstream xmlFile;
		xmlFile.open(xmlfilepath, std::ios::out | std::ios::trunc);
		if ( xmlFile.is_open() )
		{
			// write xml file
			WriteXmlFile(xmlFile);

			// and close file
			xmlFile.close();
		}
		else
		{
			std::cout << "Failed to open " << xmlfilepath << std::endl;
		}

		// and wait a bit
		for (int i=0; i<XML_UPDATE_PERIOD && keep_running; i++)
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// xml helpers

void CReflector::WriteXmlFile(std::ofstream &xmlFile)
{
	const std::string Callsign(g_CFG.GetCallsign());
	// write header
	xmlFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;

	// reflector start
	xmlFile << "<REFLECTOR CALLSIGN=\"" << Callsign << "\">" << std::endl;

	// software version
	xmlFile << "<VERSION>" << g_Version << "</VERSION>" << std::endl;

	// linked peers
	xmlFile << "<PEERS>" << std::endl;
	// lock
	auto peers = GetPeers();
	// iterate on peers
	for ( auto pit=peers->cbegin(); pit!=peers->cend(); pit++ )
	{
		(*pit)->WriteXml(xmlFile);
	}
	// unlock
	ReleasePeers();
	xmlFile << "</PEERS>" << std::endl;

	// linked nodes
	xmlFile << "<NODES>" << std::endl;
	// lock
	auto clients = GetClients();
	// iterate on clients
	for ( auto cit=clients->cbegin(); cit!=clients->cend(); cit++ )
	{
		if ( (*cit)->IsNode() && (*cit)->GetCallsign().GetCS(4).compare("M17-") )
		{
			(*cit)->WriteXml(xmlFile);
		}
	}
	// unlock
	ReleaseClients();
	xmlFile << "</NODES>" << std::endl;

	// last heard users
	xmlFile << "<STATIONS>" << std::endl;
	// lock
	CUsers *users = GetUsers();
	// iterate on users
	for ( auto it=users->begin(); it!=users->end(); it++ )
	{
		it->WriteXml(xmlFile);
	}
	// unlock
	ReleaseUsers();
	xmlFile << "</STATIONS>" << std::endl;

	// reflector end
	xmlFile << "</REFLECTOR>" << std::endl;
}

#ifndef NO_DHT

// DHT put() and get()
void CReflector::PutDHTPeers()
{
	const std::string cs(g_CFG.GetCallsign());
	// load it up
	SMrefdPeers1 p;
	time(&p.timestamp);
	p.sequence = peers_put_count++;
	auto peers = GetPeers();
	for (auto pit=peers->cbegin(); pit!=peers->cend(); pit++)
	{
		p.list.emplace_back((*pit)->GetCallsign().GetCS(), (*pit)->GetReflectorModules(), (*pit)->GetConnectTime());
	}
	ReleasePeers();

	auto nv = std::make_shared<dht::Value>(p);
	nv->user_type.assign("mrefd-peers-1");
	nv->id = toUType(EMrefdValueID::Peers);

	node.putSigned(
		refhash,
		nv,
		[](bool success){ std::cout << "PutDHTPeers() " << (success ? "successful" : "unsuccessful") << std::endl; },
		true	// permanent!
	);
}

void CReflector::PutDHTConfig()
{
	const std::string cs(g_CFG.GetCallsign());
	SMrefdConfig1 cfg;
	time(&cfg.timestamp);
	cfg.callsign.assign(cs);
	cfg.ipv4addr.assign(g_CFG.GetIPv4ExtAddr());
	cfg.ipv6addr.assign(g_CFG.GetIPv6ExtAddr());
	cfg.modules.assign(g_CFG.GetModules());
	cfg.encryptedmods.assign(g_CFG.GetEncryptedMods());
	cfg.url.assign(g_CFG.GetURL());
	cfg.email.assign(g_CFG.GetEmailAddr());
	cfg.country.assign(g_CFG.GetCountry());
	cfg.sponsor.assign(g_CFG.GetSponsor());
	std::ostringstream ss;
	ss << g_Version;
	cfg.version.assign(ss.str());
	cfg.port = (unsigned short)g_CFG.GetPort();

	auto nv = std::make_shared<dht::Value>(cfg);
	nv->user_type.assign("mrefd-config-1");
	nv->id = toUType(EMrefdValueID::Config);

	node.putSigned(
		refhash,
		nv,
		[](bool success){ std::cout << "PutDHTConfig() " << (success ? "successful" : "unsuccessful") << std::endl; },
		true
	);
}

void CReflector::GetDHTConfig(const std::string &cs)
{
	static SMrefdConfig1 cfg;
	cfg.timestamp = 0;	// every time this is called, zero the timestamp
	auto item = g_IFile.FindMapItem(cs);
	if (nullptr == item)
	{
		std::cerr << "Can't Listen() for " << cs << " because it doesn't exist" << std::endl;
		return;
	}
	std::cout << "Getting " << cs << " connection info..." << std::endl;

	// we only want the configuration section of the reflector's document
	dht::Where w;
	w.id(toUType(EMrefdValueID::Config));

	node.get(
		dht::InfoHash::get(cs),
		[](const std::shared_ptr<dht::Value> &v) {
			if (0 == v->user_type.compare(MREFD_CONFIG_1))
			{
				auto rdat = dht::Value::unpack<SMrefdConfig1>(*v);
				if (rdat.timestamp > cfg.timestamp)
				{
					// the time stamp is the newest so far, so put it in the static cfg struct
					cfg = dht::Value::unpack<SMrefdConfig1>(*v);
				}
			}
			else
			{
				std::cerr << "Get() returned unknown user_type: '" << v->user_type << "'" << std::endl;
			}
			return true;	// check all the values returned
		},
		[](bool success) {
			if (success)
			{
				if (cfg.timestamp)
				{
					// if the get() call was successful and there is a nonzero timestamp, then do the update
					g_IFile.Update(cfg.callsign, cfg.modules, cfg.ipv4addr, cfg.ipv6addr, cfg.port, cfg.encryptedmods);
				}
			}
			else
			{
				std::cout << "Get() was unsuccessful" << std::endl;
			}
		},
		{}, // empty filter
		w	// just the configuration section
	);
}

#endif
