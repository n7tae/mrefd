//
//  cpeer.h
//  M17Refd
//
//  Created by Jean-Luc Deltombe (LX3JL) on 10/12/2016.
//  Copyright © 2016 Jean-Luc Deltombe (LX3JL). All rights reserved.
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

#include "version.h"
#include "timer.h"
#include "ip.h"
#include "callsign.h"
#include "client.h"

////////////////////////////////////////////////////////////////////////////////////////
//

////////////////////////////////////////////////////////////////////////////////////////
// class

class CPeer
{
public:
	// constructors
	CPeer();
	CPeer(const CCallsign &, const CIp &, const char *);
	CPeer(const CPeer &) = delete;

	// destructor
	virtual ~CPeer();

	// operators
	bool operator ==(const CPeer &) const;

	// get
	const CCallsign &GetCallsign(void) const            { return m_Callsign; }
	const CIp &GetIp(void) const                        { return m_Ip; }
	char *GetReflectorModules(void)                     { return m_ReflectorModules; }

	// set

	// identity
	int GetProtocolRevision(const CVersion &ver) const;
	const char *GetProtocolName(void) const     { return "M17"; }

	// status
	bool IsTransmitting(void) const;
	void Alive(void);
	bool IsAlive(void) const;
	void Heard(void)                            { m_LastHeardTime = std::time(nullptr); }

	// clients access
	int     GetNbClients(void) const                    { return (int)m_Clients.size(); }
	void    ClearClients(void)                          { m_Clients.clear(); }

	// pass-thru
	std::list<std::shared_ptr<CClient>>::iterator begin()              { return m_Clients.begin(); }
	std::list<std::shared_ptr<CClient>>::iterator end()                { return m_Clients.end(); }
	std::list<std::shared_ptr<CClient>>::const_iterator cbegin() const { return m_Clients.cbegin(); }
	std::list<std::shared_ptr<CClient>>::const_iterator cend() const   { return m_Clients.cend(); }

	// reporting
	virtual void WriteXml(std::ofstream &);

protected:
	// data
	CCallsign m_Callsign;
	CIp       m_Ip;
	char      m_ReflectorModules[27];
	std::list<std::shared_ptr<CClient>> m_Clients;

	// status
	CTimer      m_LastKeepaliveTime;
	std::time_t m_ConnectTime;
	std::time_t m_LastHeardTime;
};
