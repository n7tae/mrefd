//
//  creflector.cpp
//  M17Refd
//
//  Created by Jean-Luc Deltombe (LX3JL) on 31/10/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
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
#include <string.h>
#include "reflector.h"
#include "gatekeeper.h"

////////////////////////////////////////////////////////////////////////////////////////
// constructor

CReflector::CReflector()
{
	keep_running = true;
}

CReflector::CReflector(const CCallsign &callsign)
{
	keep_running = true;
	m_Callsign = callsign;
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
	for ( int i = 0; i < NB_OF_MODULES; i++ )
	{
		if ( m_RouterFuture[i].valid() )
		{
			m_RouterFuture[i].get();
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////
// operation

bool CReflector::Start(void)
{
	// let's go!
	keep_running = true;

	// init gate keeper. It can only return true!
	g_GateKeeper.Init();

	// create protocols
	if (! m_Protocol.Initialize(PROTOCOL_M17, M17_PORT, true, true))
	{
		m_Protocol.Close();
		return false;
	}

	// start one thread per reflector module
	for ( int i = 0; i < NB_OF_MODULES; i++ )
	{
		m_RouterFuture[i] = std::async(std::launch::async, &CReflector::RouterThread, this, &(m_Stream[i]));
	}

	// start the reporting threads
	m_XmlReportFuture = std::async(std::launch::async, &CReflector::XmlReportThread, this);

	return true;
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
#ifdef JSON_MONITOR
	if ( m_JsonReportFuture.valid() )
	{
		m_JsonReportFuture.get();
	}
#endif

	// stop & delete all router thread
	for ( int i = 0; i < NB_OF_MODULES; i++ )
	{
		if ( m_RouterFuture[i].valid() )
		{
			m_RouterFuture[i].get();
		}
	}

	// close protocols
	m_Protocol.Close();

	// close gatekeeper
	g_GateKeeper.Close();
}

////////////////////////////////////////////////////////////////////////////////////////
// stream opening & closing

bool CReflector::IsStreaming(char module)
{
	return false;
}

// clients MUST have been locked by the caller so we can freely access it within the fuction
CPacketStream *CReflector::OpenStream(std::unique_ptr<CPacket> &Header, std::shared_ptr<CClient>client)
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

	if ( client->IsAMaster() )
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

	CPacketStream *stream = GetStream(module);
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
		client->SetMasterOfModule(module);

		// update last heard time
		client->Heard();

		// report
		std::cout << "Opening stream on module " << module << " for client " << client->GetCallsign() << " with id 0x" << std::hex << Header->GetStreamId() << std::dec << " by user " << Header->GetSourceCallsign() << std::endl;

		// and push header packet
		stream->Push(std::move(Header));

		// notify
		g_Reflector.OnStreamOpen(stream->GetUserCallsign());

	}
	else
	{
		std::cerr << "Can't open a packet stream for " << client->GetCallsign() << std::endl;
	}
	stream->Unlock();
	return stream;
}

void CReflector::CloseStream(CPacketStream *stream)
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
				CTimePoint::TaskSleepFor(10);
		}
		while (!bEmpty);

		GetClients();	// lock clients
		stream->Lock();	// lock stream

		// get and check the master
		std::shared_ptr<CClient>client = stream->GetOwnerClient();
		if ( client != nullptr )
		{
			// client no longer a master
			client->NotAMaster();

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

void CReflector::RouterThread(CPacketStream *streamIn)
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
			CTimePoint::TaskSleepFor(10);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// report threads

void CReflector::XmlReportThread()
{
	while (keep_running)
	{
		// report to xml file
		std::ofstream xmlFile;
		xmlFile.open(XML_PATH, std::ios::out | std::ios::trunc);
		if ( xmlFile.is_open() )
		{
			// write xml file
			WriteXmlFile(xmlFile);

			// and close file
			xmlFile.close();
		}
		else
		{
			std::cout << "Failed to open " << XML_PATH  << std::endl;
		}

		// and wait a bit
		for (int i=0; i< XML_UPDATE_PERIOD && keep_running; i++)
			CTimePoint::TaskSleepFor(1000);
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

int CReflector::GetModuleIndex(char module) const
{
	int i = (int)module - (int)'A';
	if ( (i < 0) || (i >= NB_OF_MODULES) )
	{
		i = -1;
	}
	return i;
}

CPacketStream *CReflector::GetStream(char module)
{
	int i = GetModuleIndex(module);
	if ( i >= 0 )
	{
		return &(m_Stream[i]);
	}
	return nullptr;
}

bool CReflector::IsStreamOpen(const std::unique_ptr<CPacket> &DvHeader)
{
	for ( unsigned i = 0; i < m_Stream.size(); i++  )
	{
		if ( (m_Stream[i].GetPacketStreamId() == DvHeader->GetStreamId()) && (m_Stream[i].IsOpen()) )
			return true;
	}
	return false;
}

char CReflector::GetStreamModule(CPacketStream *stream)
{
	for ( unsigned i = 0; i < m_Stream.size(); i++ )
	{
		if ( &(m_Stream[i]) == stream )
			return GetModuleLetter(i);
	}
	return ' ';
}

////////////////////////////////////////////////////////////////////////////////////////
// xml helpers

void CReflector::WriteXmlFile(std::ofstream &xmlFile)
{
	// write header
	xmlFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;

	// software version
	char sz[64];
	::sprintf(sz, "%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION);
	xmlFile << "<Version>" << sz << "</Version>" << std::endl;

	// linked peers
	xmlFile << "<" << m_Callsign << " linked peers>" << std::endl;
	// lock
	CPeers *peers = GetPeers();
	// iterate on peers
	for ( auto pit=peers->cbegin(); pit!=peers->cend(); pit++ )
	{
		(*pit)->WriteXml(xmlFile);
	}
	// unlock
	ReleasePeers();
	xmlFile << "</" << m_Callsign << " linked peers>" << std::endl;

	// linked nodes
	xmlFile << "<" << m_Callsign << " linked nodes>" << std::endl;
	// lock
	CClients *clients = GetClients();
	// iterate on clients
	for ( auto cit=clients->cbegin(); cit!=clients->cend(); cit++ )
	{
		if ( (*cit)->IsNode() )
		{
			(*cit)->WriteXml(xmlFile);
		}
	}
	// unlock
	ReleaseClients();
	xmlFile << "</" << m_Callsign << " linked nodes>" << std::endl;

	// last heard users
	xmlFile << "<" << m_Callsign << " heard users>" << std::endl;
	// lock
	CUsers *users = GetUsers();
	// iterate on users
	for ( auto it=users->begin(); it!=users->end(); it++ )
	{
		(*it).WriteXml(xmlFile);
	}
	// unlock
	ReleaseUsers();
	xmlFile << "</" << m_Callsign << " heard users>" << std::endl;
}
