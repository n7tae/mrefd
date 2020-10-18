//
//  Created by Jean-Luc Deltombe (LX3JL) on 01/11/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020 Thomas A. Early N7TAE
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
//    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#pragma once

#include "timepoint.h"
#include "protocol.h"
#include "packet.h"
#include "crc.h"

////////////////////////////////////////////////////////////////////////////////////////
// class

class CM17Protocol : public CProtocol
{
public:
	// initialization
	bool Initialize(const int ptype, const uint16_t port, const bool has_ipv4, const bool has_ipv6);

	// task
	void Task(void);

protected:
	// queue helper
	void HandleQueue(void);

	// keepalive helpers
	void HandlePeerLinks(void);
	void HandleKeepalives(void);

	// stream helpers
	void OnFirstPacketIn(std::unique_ptr<CPacket> &, const CIp &);

	// packet decoding helpers
	bool IsValidConnect(const uint8_t *, CCallsign &, char *);
	bool IsValidDisconnect(const uint8_t *, CCallsign &);
	bool IsValidKeepAlive(const uint8_t *, CCallsign &);
	bool IsValidPacket(const uint8_t *, std::unique_ptr<CPacket> &);

	// packet encoding helpers
	void EncodeKeepAlivePacket(uint8_t *);
	void EncodeConnectPacket(uint8_t *, const char *);
	void EncodeConnectAckPacket(uint8_t *);
	void EncodeConnectNackPacket(uint8_t *);
	void EncodeDisconnectPacket(uint8_t *, char);
	void EncodeDisconnectedPacket(uint8_t *);

protected:
	// time
	CTimePoint m_LastKeepaliveTime;
	CTimePoint m_LastPeersLinkTime;

private:
	CCRC crc;
};
