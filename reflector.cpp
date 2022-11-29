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
	refID = dht::crypto::generateIdentity(g_CFG.GetCallsign());
	privateKey = dht::crypto::PrivateKey::generate();
	node.run(17171, refID, true);
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

// clients MUST have been locked by the caller so we can freely access it within the fuction
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

		// notify
		g_Reflector.OnStreamOpen(stream->GetUserCallsign());

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

			// notify
			OnStreamClose(stream->GetUserCallsign());

			std::cout << "Closing stream on module " << GetStreamModule(stream) << std::endl;
		}

		// release clients
		ReleaseClients();

		// unlock before closing
		// to avoid double lock in assiociated
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
	// get our module
	uint8_t uiModuleId = GetStreamModule(streamIn);

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
// notifications

void CReflector::OnPeersChanged(void)
{
	CNotification notification(NOTIFICATION_PEERS);

	m_Notifications.Lock();
	m_Notifications.push(notification);
	m_Notifications.Unlock();
}

void CReflector::OnClientsChanged(void)
{
	CNotification notification(NOTIFICATION_CLIENTS);

	m_Notifications.Lock();
	m_Notifications.push(notification);
	m_Notifications.Unlock();
}

void CReflector::OnUsersChanged(void)
{
	CNotification notification(NOTIFICATION_USERS);

	m_Notifications.Lock();
	m_Notifications.push(notification);
	m_Notifications.Unlock();
}

void CReflector::OnStreamOpen(const CCallsign &callsign)
{
	CNotification notification(NOTIFICATION_STREAM_OPEN, callsign);

	m_Notifications.Lock();
	m_Notifications.push(notification);
	m_Notifications.Unlock();
}

void CReflector::OnStreamClose(const CCallsign &callsign)
{
	CNotification notification(NOTIFICATION_STREAM_CLOSE, callsign);

	m_Notifications.Lock();
	m_Notifications.push(notification);
	m_Notifications.Unlock();
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
	CPeers *peers = GetPeers();
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
	CClients *clients = GetClients();
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
void CReflector::PutDHTInfo()
{
	const std::string cs(g_CFG.GetCallsign());
	SReflectorData1 rd;
	rd.cs.assign(cs);
	rd.ipv4.assign(g_CFG.GetIPv4ExtAddr());
	rd.ipv6.assign(g_CFG.GetIPv6ExtAddr());
	rd.mods.assign(g_CFG.GetModules());
	rd.emods.assign(g_CFG.GetEncryptedMods());
	rd.url.assign(g_CFG.GetURL());
	rd.email.assign(g_CFG.GetEmailAddr());
	rd.country.assign(g_CFG.GetCountry());
	rd.sponsor.assign(g_CFG.GetSponsor());
	rd.port = (unsigned short)g_CFG.GetPort();

	auto peers = g_Reflector.GetPeers();
	for (auto pit=peers->cbegin(); pit!=peers->cend(); pit++)
	{
		const auto modules((*pit)->GetReflectorModules());
		rd.peers.emplace_back(std::pair<std::string,std::string>((*pit)->GetCallsign().GetCS(), modules));
	}
	g_Reflector.ReleasePeers();

	auto nv = std::make_shared<dht::Value>(rd);
	nv->user_type.assign("reflector-mrefd-1");
	nv->id = 0xffffffffffffffffu;
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

void CReflector::Get(const std::string &cs)
{
	auto item = g_IFile.FindMapItem(cs);
	if (nullptr == item)
	{
		std::cerr << "Can't Listen() for " << cs << " because it doesn't exist" << std::endl;
		return;
	}
	std::cout << "Getting " << cs << " connection info..." << std::endl;
	node.get(
		dht::InfoHash::get(cs),
		[](const std::shared_ptr<dht::Value> &v) {
			if (0 == v->user_type.compare("reflector-mrefd-0"))
			{
				auto rdat = dht::Value::unpack<SReflectorData0>(*v);
				g_IFile.Update(rdat.mods, rdat.cs, rdat.ipv4, rdat.ipv6, rdat.port, ""); // TODO: this empty string shoud be "ABCDEFGHIJKLMNOPQRSTUVWXYZ", but we need to wait until everyone catches up
			}
			else if (0 == v->user_type.compare("reflector-mrefd-1"))
			{
				auto rdat = dht::Value::unpack<SReflectorData1>(*v);
				g_IFile.Update(rdat.mods, rdat.cs, rdat.ipv4, rdat.ipv6, rdat.port, rdat.emods);
			}
			else
			{
				std::cerr << "Get() returned unknown user_type: '" << v->user_type << "'" << std::endl;
			}
			return false;
		},
		[](bool success) {
			if (! success)
				std::cout << "Get() was unsuccessful" << std::endl;
		}
	);
}
#endif
