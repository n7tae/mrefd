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

#pragma once

#include "users.h"
#include "clients.h"
#include "peers.h"
#include "protocol.h"
#include "packetstream.h"
#include "notificationqueue.h"


////////////////////////////////////////////////////////////////////////////////////////
// define

// event defines
#define EVENT_NONE      0
#define EVENT_CLIENTS   1
#define EVENT_USERS     2

////////////////////////////////////////////////////////////////////////////////////////
// class

class CReflector
{
public:
	// constructor
	CReflector();

	// destructor
	virtual ~CReflector();

	// settings
	const CCallsign &GetCallsign(void) const         { return m_Callsign; }


	// operation
	bool Start(const char *cfgfilename);
	void Stop(void);

	// clients
	CClients  *GetClients(void)                     { m_Clients.Lock(); return &m_Clients; }
	void      ReleaseClients(void)                  { m_Clients.Unlock(); }

	// peers
	CPeers   *GetPeers(void)                        { m_Peers.Lock(); return &m_Peers; }
	void      ReleasePeers(void)                    { m_Peers.Unlock(); }

	// stream opening & closing
	std::shared_ptr<CPacketStream> OpenStream(std::unique_ptr<CPacket> &, std::shared_ptr<CClient>);
	bool IsStreaming(char);
	void CloseStream(std::shared_ptr<CPacketStream>);

	// users
	CUsers  *GetUsers(void)                         { m_Users.Lock(); return &m_Users; }
	void    ReleaseUsers(void)                      { m_Users.Unlock(); }

	// get
	bool IsValidModule(char c) const                { return m_Modules.npos != m_Modules.find(c); }

	// notifications
	void OnPeersChanged(void);
	void OnClientsChanged(void);
	void OnUsersChanged(void);
	void OnStreamOpen(const CCallsign &);
	void OnStreamClose(const CCallsign &);

protected:
	// threads
	void RouterThread(std::shared_ptr<CPacketStream>);
	void XmlReportThread(void);
	// streams
	std::shared_ptr<CPacketStream> GetStream(char);
	bool IsStreamOpen(const std::unique_ptr<CPacket> &);
	char GetStreamModule(std::shared_ptr<CPacketStream>);

	// xml helpers
	void WriteXmlFile(std::ofstream &);

protected:
	// identity
	const CCallsign m_Callsign;
	const std::string m_Modules;

	// objects
	CUsers    m_Users;    // sorted list of lastheard stations
	CClients  m_Clients;  // list of linked repeaters/nodes/peers's modules
	CPeers    m_Peers;    // list of linked peers
	CProtocol m_Protocol; // the only protocol
	// queues
	std::unordered_map<char, std::shared_ptr<CPacketStream>> m_Streams;
	std::unordered_map<std::shared_ptr<CPacketStream>, char> m_RStreams;

	// threads
	std::atomic<bool> keep_running;
	std::unordered_map<char, std::future<void>> m_ModuleFutures;
	std::future<void> m_XmlReportFuture, m_JsonReportFuture;

	// notifications
	CNotificationQueue  m_Notifications;
};
