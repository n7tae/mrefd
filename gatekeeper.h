//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//
//  Copyright © 2022 Thomas A. Early, N7TAE
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

#ifndef NO_DHT
#include <opendht.h>
#endif

#include "main.h"
#include "callsign.h"
#include "ip.h"
#include "bwset.h"
#include "base.h"

#ifndef NO_DHT
// the possible DHT formats

struct SReflectorData0
{
	std::string cs, ipv4;
	std::string ipv6, mods, url, email;
	uint16_t port;
	std::vector<std::pair<std::string, std::string>> peers;

	MSGPACK_DEFINE(cs, ipv4, ipv6, mods, url, email, port, peers);
};

struct SReflectorData1
{
	std::string cs, ipv4;
	std::string ipv6, mods, encryptmods, url, email;
	uint16_t port;
	std::vector<std::pair<std::string, std::string>> peers;

	MSGPACK_DEFINE(cs, ipv4, ipv6, mods, encryptmods, url, email, port, peers);
};

#endif

////////////////////////////////////////////////////////////////////////////////////////
// class

class CGateKeeper : public CBase
{
public:
	// constructor
	CGateKeeper();

	// destructor
	~CGateKeeper();

	// init & clode
	bool Init(void);
	void Close(void);

	// Publish DHT
	#ifndef NO_DHT
	void PutDHTInfo();
	void Listen(const std::string &cs);
	void CancelListen(const std::string &cs);
	#endif

	// authorizations
	bool MayLink(const CCallsign &, const CIp &, char * = nullptr) const;
	bool MayTransmit(const CCallsign &, const CIp &) const;

protected:
	// thread
	void Thread();

	// operation helpers
	bool IsNodeListedOk(const CCallsign &) const;
	bool IsPeerListedOk(const CCallsign &, const CIp &, const char *) const;

	// data
	CBWSet   m_NodeWhiteSet;
	CBWSet   m_NodeBlackSet;

	// thread
	std::atomic<bool> keep_running;
	std::future<void> m_Future;

	// Distributed Hash Table
	#ifndef NO_DHT
	dht::DhtRunner node;
	dht::crypto::Identity refID;
	dht::crypto::PrivateKey privateKey;
	#endif
};
