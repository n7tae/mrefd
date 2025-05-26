//
//  Created by Jean-Luc Deltombe (LX3JL) on 31/10/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020 Thomas A. Early N7TAE
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

#include "udpsocket.h"
#include "callsign.h"
#include "packet.h"
#include "timer.h"
#include "ip.h"

////////////////////////////////////////////////////////////////////////////////////////
//

////////////////////////////////////////////////////////////////////////////////////////
// class

class CClient
{
public:
	// constructors
	CClient() = delete;
	CClient(const CClient &) = delete;
	CClient(const CCallsign cs, const CIp ip, char mod, const CUdpSocket &sock, bool listenOnly = false);

	// destructor
	virtual ~CClient() {};

	// operators
	bool operator ==(const CClient &) const;

	// get
	const CCallsign &GetCallsign(void) const { return m_Callsign; }
	const CIp &GetIp(void) const             { return m_Ip; }
	char GetModule(void) const               { return m_Callsign.GetModule(); }
	std::time_t GetConnectTime(void) const   { return m_ConnectTime; }
	std::time_t GetLastHeardTime(void) const { return m_LastHeardTime; }
	char GetReflectorModule(void) const      { return m_ReflectorModule; }

	// identity
	const char *GetProtocolName(void) const  { return "M17"; }
	bool IsNode(void) const                  { return true; }
	bool IsListenOnly(void) const            { return m_ListenOnly; }

	// status
	void Alive(void);
	bool IsAlive(void) const;
	bool IsTransmitting(void) const          { return (m_isTXing); }
	void SetTX(void)                         { m_isTXing = true; }
	void ClearTX(void)                       { m_isTXing = false; }
	void Heard(void)                         { m_LastHeardTime = std::time(nullptr); }

	// function
	void SendPacket(const CPacket &pack) const;
	// reporting
	void WriteXml(std::ofstream &);

protected:
	const CCallsign   m_Callsign;
	const CIp         m_Ip;
	const char        m_ReflectorModule;
	const CUdpSocket &m_Sock;
	const bool        m_ListenOnly;

	// status
	bool        m_isTXing;
	CTimer		m_LastKeepaliveTime;
	std::time_t m_ConnectTime;
	std::time_t m_LastHeardTime;
};
