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

#pragma once

#include <unordered_map>

#include "users.h"
#include "clients.h"
#include "peers.h"
#include "protocol.h"
#include "packetstream.h"

#ifndef NO_DHT
#include "dht-values.h"
#endif

class CReflector
{
public:
	// constructor
	CReflector();

	// destructor
	virtual ~CReflector();

	// operation
	bool Start(const char *cfgfilename);
	void Stop(void);

	// clients
	CClients  *GetClients(void)                     { m_Clients.Lock(); return &m_Clients; }
	void      ReleaseClients(void)                  { m_Clients.Unlock(); }

	// peers
	CPeers   &GetPeers(void)                        { return m_Peers; }

	// users
	CUsers  *GetUsers(void)                         { m_Users.Lock(); return &m_Users; }
	void    ReleaseUsers(void)                      { m_Users.Unlock(); }

#ifndef NO_DHT
	// Publish DHT
	void PutDHTConfig();
	void PutDHTPeers();
	void GetDHTConfig(const std::string &cs);
#endif

protected:
	// thread
	void XmlReportThread(void);

	// xml helpers
	void WriteXmlFile(std::ofstream &);

protected:
	// objects
	CUsers    m_Users;    // sorted list of lastheard stations
	CClients  m_Clients;  // list of linked repeaters/nodes/peers's modules
	CPeers    m_Peers;    // list of linked peers
	CProtocol m_Protocol; // the only protocol

	// threads
	std::atomic<bool> keep_running;
	std::future<void> m_XmlReportFuture, m_JsonReportFuture;

	// Distributed Hash Table
#ifndef NO_DHT
	dht::DhtRunner node;
	dht::InfoHash refhash;
	unsigned int peers_put_count;
#endif
};
