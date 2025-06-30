//
//  cpeer.h
//  mrefd
//
//  Created by Jean-Luc Deltombe (LX3JL) on 10/12/2016.
//  Copyright © 2016 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020,2022,2025 Thomas A. Early, N7TAE
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

#include <list>
#include <memory>

#include "version.h"
#include "timer.h"
#include "ip.h"
#include "callsign.h"
#include "client.h"
#include "udpsocket.h"

class CPeer
{
public:
	// constructors
	CPeer() = delete;
	CPeer(const CCallsign cs, const CIp ip, EClientType type, const std::string &mods, const CUdpSocket &sock);
	CPeer(const CPeer &) = delete;

	// destructor
	virtual ~CPeer();

	// get
	const CCallsign &GetCallsign(void) const        { return m_Callsign; }
	const CIp &GetIp(void) const                    { return m_Ip; }
	const std::string &GetSharedModules(void) const { return m_sharedModules; }
	std::time_t GetConnectTime(void) const          { return m_ConnectTime; }

	// set

	// identity
	const char *GetProtocolName(void) const { return "M17"; }

	// status
	bool IsTransmitting(void) const;
	void Alive(void);
	bool IsAlive(void) const;

	// clients access
	int     GetNbClients(void) const { return (int)m_Clients.size(); }
	void    ClearClients(void)       { m_Clients.clear(); }

	// pass-through
	std::list<SPClient>::iterator begin()              { return m_Clients.begin(); }
	std::list<SPClient>::iterator end()                { return m_Clients.end(); }
	std::list<SPClient>::const_iterator cbegin() const { return m_Clients.cbegin(); }
	std::list<SPClient>::const_iterator cend() const   { return m_Clients.cend(); }

	// reporting
	virtual void WriteXml(std::ofstream &) const;

protected:
	// data
	const CCallsign   m_Callsign;
	const CIp         m_Ip;
	const EClientType m_refType;
	const std::string m_sharedModules;
	std::list<SPClient> m_Clients;

	// status
	CTimer      m_LastKeepaliveTime;
	std::time_t m_ConnectTime;
};
