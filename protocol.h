//
//  cprotocol.h
//  mrefd
//
//  Created by Jean-Luc Deltombe (LX3JL) on 01/11/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2022 Thomas A. Early, N7TAE
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

#include <regex>
#include "udpsocket.h"
#include "packetstream.h"
#include "packet.h"
#include "base.h"
#include "crc.h"


////////////////////////////////////////////////////////////////////////////////////////
// class

class CProtocol : public CBase
{
public:
	// constructor
	CProtocol();

	// destructor
	virtual ~CProtocol();

	// initialization
	bool Initialize(const uint16_t port, const std::string &ipv4bind, const std::string &ipv6bind);
	void Close(void);

	// queue
	CPacketQueue *GetQueue(void)        { m_Queue.Lock(); return &m_Queue; }
	void ReleaseQueue(void)             { m_Queue.Unlock(); }

	// get
	const CCallsign &GetReflectorCallsign(void)const { return m_ReflectorCallsign; }

	// task
	void Thread(void);
	void Task(void);

protected:
	// queue helper
	void HandleQueue(void);

	// keepalive helpers
	void HandlePeerLinks(void);
	void HandleKeepalives(void);

	// stream helpers
	void OnPacketIn(std::unique_ptr<CPacket> &, const CIp &);
	void OnFirstPacketIn(std::unique_ptr<CPacket> &, const CIp &);

	// packet decoding helpers
	bool IsValidConnect(const uint8_t *, CCallsign &, char *);
	bool IsValidDisconnect(const uint8_t *, CCallsign &);
	bool IsValidKeepAlive(const uint8_t *, CCallsign &);
	bool IsValidPacket(const uint8_t *, bool is_internal, std::unique_ptr<CPacket> &);
	bool IsValidNAcknowledge(const uint8_t *, CCallsign &);
	bool IsValidInterlinkConnect(const uint8_t *, CCallsign &, char *);
	bool IsVaildInterlinkAcknowledge(const uint8_t *, CCallsign &, char *);

	// packet encoding helpers
	void EncodeKeepAlivePacket(uint8_t *);
	void EncodeConnectAckPacket(uint8_t *);
	void EncodeConnectNackPacket(uint8_t *);
	void EncodeDisconnectPacket(uint8_t *, char);
	void EncodeDisconnectedPacket(uint8_t *);
	void EncodeInterlinkConnectPacket(SInterConnect &, const std::string &);
	void EncodeInterlinkAckPacket(SInterConnect &, const char *);
	void EncodeInterlinkNackPacket(uint8_t *);

	// stream handle helpers
	std::shared_ptr<CPacketStream> GetStream(uint16_t, const CIp &);
	void CheckStreamsTimeout(void);

	// syntax helper
	bool IsNumber(char) const;
	bool IsLetter(char) const;
	bool IsSpace(char) const;

	ssize_t Receive6(uint8_t *buf, CIp &Ip, int time_ms);
	ssize_t Receive4(uint8_t *buf, CIp &Ip, int time_ms);
	ssize_t ReceiveDS(uint8_t *buf, CIp &Ip, int time_ms);
	ssize_t (CProtocol::*Receive)(uint8_t *buf, CIp &Ip, int time_ms);

	void Send(const char    *buf, const CIp &Ip) const;
	void Send(const uint8_t *buf, size_t size, const CIp &Ip) const;
	void Send(const char    *buf, const CIp &Ip, uint16_t port) const;
	void Send(const uint8_t *buf, size_t size, const CIp &Ip, uint16_t port) const;

	// socket
	CUdpSocket m_Socket4;
	CUdpSocket m_Socket6;

	// streams
	std::list<std::shared_ptr<CPacketStream>> m_Streams;

	// queue
	CPacketQueue    m_Queue;

	// thread
	std::atomic<bool> keep_running;
	std::future<void> m_Future;

	// identity
	CCallsign       m_ReflectorCallsign;

	// debug
	CTimer      m_DebugTimer;

	// time
	CTimer m_LastKeepaliveTime;
	CTimer m_LastPeersLinkTime;

	// for PutDHTInfo
	bool publish;

private:
	std::regex clientRegEx, peerRegEx;
	CCRC crc;
};
