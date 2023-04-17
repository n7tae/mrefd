//
//  creflector.cpp
//  M17Refd
//
//  Created by Jean-Luc Deltombe (LX3JL) on 31/10/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020,2022 Thomas A. Early, N7TAE
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
	peers_put_count = clients_put_count = users_put_count = 0;
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
	for (auto &f : m_ModuleFutures)
	{
		if (f.second.valid())
		{
			f.second.get();
		}
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
	node.run(17171, dht::crypto::generateIdentity(g_CFG.GetCallsign()), true);
	node.bootstrap(g_CFG.GetBootstrap(), "17171");
#endif

	// create protocols
	if (! m_Protocol.Initialize(g_CFG.GetPort(), g_CFG.GetIPv4BindAddr(), g_CFG.GetIPv6BindAddr()))
	{
		m_Protocol.Close();
		return true;
	}

	// start one thread per reflector module
	for (const auto &m : g_CFG.GetModules())
	{
		auto stream = std::make_shared<CPacketStream>();
		m_Streams[m] = stream;
		m_RStreams[stream] = m;
		m_ModuleFutures[m] = std::async(std::launch::async, &CReflector::RouterThread, this, stream);
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

	// stop & delete all router thread
	for (auto &f : m_ModuleFutures)
	{
		if (f.second.valid() )
		{
			f.second.get();
		}
	}

	// close protocols
	m_Protocol.Close();

	// close gatekeeper
	g_GateKeeper.Close();

#ifndef NO_DHT
	// kill the DHT
	node.cancelPut(refhash, toUType(EMrefdValueID::Config));
	node.cancelPut(refhash, toUType(EMrefdValueID::Peers));
	node.cancelPut(refhash, toUType(EMrefdValueID::Clients));
	node.cancelPut(refhash, toUType(EMrefdValueID::Users));
	node.shutdown({}, true);
	node.join();
#endif
}

////////////////////////////////////////////////////////////////////////////////////////
// stream opening & closing

bool CReflector::IsStreaming(char module)
{
	return false;
}

// clients MUST have been locked by the caller so we can freely access it within the function
std::shared_ptr<CPacketStream> CReflector::OpenStream(std::unique_ptr<CPacket> &Header, std::shared_ptr<CClient>client)
{
	// check sid is not zero
	if ( 0U == Header->GetStreamId() )
	{
		std::cerr << "Incoming stream has zero streamID" << std::endl;
		return nullptr;
	}

	// check if client is valid candidate
	if ( ! m_Clients.IsClient(client) )
	{
		std::cerr << "can't find client " << client->GetCallsign() << std::endl;
		return nullptr;
	}

	if ( client->IsTransmitting() )
	{
		std::cerr << "Client " << client->GetCallsign() << " is already a Master" << std::endl;
		return nullptr;
	}

	// get the module's queue
	char module = Header->GetDestModule();

	// check if no stream with same streamid already open
	// to prevent loops
	if ( IsStreamOpen(Header) )
	{
		std::cerr << "Detected stream loop on module " << module << " for client " << client->GetCallsign() << " with sid " << Header->GetStreamId() << std::endl;
		return nullptr;
	}

	auto stream = GetStream(module);
	if ( stream == nullptr ) {
		std::cerr << "Can't get stream from module '" << module << "'" << std::endl;
		return nullptr;
	}

	stream->Lock();
	// is it available ?
	if ( stream->OpenPacketStream(*Header, client) )
	{
		// stream open, mark client as master
		// so that it can't be deleted
		client->SetTXModule(module);

		// update last heard time
		client->Heard();

		// report
		std::cout << "Opening stream on module " << module << " for client " << client->GetCallsign() << " with id 0x" << std::hex << Header->GetStreamId() << std::dec << " by user " << Header->GetSourceCallsign() << std::endl;

		// and push header packet
		stream->Push(std::move(Header));
	}
	stream->Unlock();
	return stream;
}

void CReflector::CloseStream(std::shared_ptr<CPacketStream> stream)
{
	if ( stream != nullptr )
	{
		// wait queue is empty. this waits forever
		bool bEmpty = false;
		do
		{
			stream->Lock();
			// do not use stream->IsEmpty() has this "may" never succeed
			// and anyway, the DvLastFramPacket short-circuit the transcoder
			// loop queues
			bEmpty = stream->empty();
			stream->Unlock();
			if ( !bEmpty )
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		while (!bEmpty);

		GetClients();	// lock clients
		stream->Lock();	// lock stream

		// get and check the master
		std::shared_ptr<CClient>client = stream->GetOwnerClient();
		if ( client != nullptr )
		{
			// client no longer a master
			client->ClearTX();

			std::cout << "Closing stream on module " << GetStreamModule(stream) << std::endl;
		}

		// release clients
		ReleaseClients();

		// unlock before closing
		// to avoid double lock in associated
		// codecstream close/thread-join
		stream->Unlock();

		// and stop the queue
		stream->ClosePacketStream();
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// router threads

void CReflector::RouterThread(std::shared_ptr<CPacketStream> streamIn)
{
	// get on input queue
	std::unique_ptr<CPacket> packet;

	while (keep_running)
	{
		// any packet in our input queue ?
		streamIn->Lock();
		if ( !streamIn->empty() )
		{
			// get the packet
			packet = streamIn->front();
			streamIn->pop();
		}
		else
		{
			packet = nullptr;
		}
		streamIn->Unlock();

		// route it
		if ( packet != nullptr )
		{
			// duplicate packet
			auto packetClone = packet->Duplicate();

			// and push it
			CPacketQueue *queue = m_Protocol.GetQueue();
			queue->push(packetClone);
			m_Protocol.ReleaseQueue();
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
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
// modules & queues

std::shared_ptr<CPacketStream> CReflector::GetStream(char module)
{
	auto it = m_Streams.find(module);
	if (m_Streams.end() == it)
		return nullptr;
	else
		return it->second;
}

bool CReflector::IsStreamOpen(const std::unique_ptr<CPacket> &DvHeader)
{
	for (auto &s : m_Streams)
	{
		if ( (s.second->GetPacketStreamId() == DvHeader->GetStreamId()) && (s.second->IsOpen()) )
			return true;
	}
	return false;
}

char CReflector::GetStreamModule(std::shared_ptr<CPacketStream> stream)
{
	auto it = m_RStreams.find(stream);
	if (m_RStreams.end() == it)
		return '\0';
	else
		return it->second;
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

void CReflector::PutDHTClients()
{
	const std::string cs(g_CFG.GetCallsign());
	SMrefdClients1 c;
	time(&c.timestamp);
	c.sequence = clients_put_count++;
	auto clients = GetClients();
	for (auto cit=clients->cbegin(); cit!=clients->cend(); cit++)
	{
		c.list.emplace_back((*cit)->GetCallsign().GetCS(), std::string((*cit)->GetIp().GetAddress()), (*cit)->GetReflectorModule(), (*cit)->GetConnectTime(), (*cit)->GetLastHeardTime());
	}
	ReleaseClients();

	auto nv = std::make_shared<dht::Value>(c);
	nv->user_type.assign("mrefd-clients-1");
	nv->id = toUType(EMrefdValueID::Clients);

	node.putSigned(
		refhash,
		nv,
		[](bool success){ std::cout << "PutDHTClients() " << (success ? "successful" : "unsuccessful") << std::endl; },
		true	// permanent!
	);
}

void CReflector::PutDHTUsers()
{
	const std::string cs(g_CFG.GetCallsign());
	SMrefdUsers1 u;
	time(&u.timestamp);
	u.sequence = users_put_count++;
	auto users = GetUsers();
	for (auto uit=users->cbegin(); uit!=users->cend(); uit++)
	{
		u.list.emplace_back((*uit).GetSource(), std::string((*uit).GetDestination()), (*uit).GetReflector(), (*uit).GetLastHeardTime());
	}
	ReleaseUsers();

	auto nv = std::make_shared<dht::Value>(u);
	nv->user_type.assign("mrefd-users-1");
	nv->id = toUType(EMrefdValueID::Users);

	node.putSigned(
		refhash,
		nv,
		[](bool success){ std::cout << "PutDHTUsers() " << (success ? "successful" : "unsuccessful") << std::endl; },
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
