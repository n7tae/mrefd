//
//  Created by Jean-Luc Deltombe (LX3JL) on 31/10/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020 Thomas A. Early N7TAE
//
// ----------------------------------------------------------------------------
//    This file is part of m17ref.
//
//    m17ref is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    m17ref is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    with this software.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#pragma once

#include "timer.h"
#include "ip.h"
#include "callsign.h"

////////////////////////////////////////////////////////////////////////////////////////
//

////////////////////////////////////////////////////////////////////////////////////////
// class

class CClient
{
public:
	// constructors
	CClient();
	CClient(const CCallsign &, const CIp &, char);
	CClient(const CClient &);
    CClient(const CCallsign &callsign, const CIp &ip, char reflectorModule, bool listenOnly);

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
	bool HasReflectorModule(void) const      { return m_ReflectorModule != ' '; }
	char GetReflectorModule(void) const      { return m_ReflectorModule; }

	// set
	void SetModule(char c)                   { m_Callsign.SetModule(c); }
	void SetReflectorModule(char c)          { m_ReflectorModule = c; }

	// identity
	const char *GetProtocolName(void) const  { return "M17"; }
	bool IsNode(void) const                  { return true; }
	bool IsListenOnly(void) const            { return m_ListenOnly; }

	// status
	void Alive(void);
	bool IsAlive(void) const;
	bool IsTransmitting(void) const          { return (m_TXModule != ' '); }
	void SetTXModule(char c)                 { m_TXModule = c; }
	void ClearTX(void)                       { m_TXModule = ' '; }
	void Heard(void)                         { m_LastHeardTime = std::time(nullptr); }

	// reporting
	void WriteXml(std::ofstream &);

protected:
	// data
	CCallsign   m_Callsign;
	CIp         m_Ip;

	// linked to
	char        m_ReflectorModule;

	// status
	char        m_TXModule;	// ' ' means client is not transmitting
	CTimer		m_LastKeepaliveTime;
	std::time_t m_ConnectTime;
	std::time_t m_LastHeardTime;

    // identity
    bool         m_ListenOnly;
};
