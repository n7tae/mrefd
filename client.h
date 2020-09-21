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

#include "timepoint.h"
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

	// destructor
	virtual ~CClient() {};

	// operators
	bool operator ==(const CClient &) const;

	// get
	const CCallsign &GetCallsign(void) const            { return m_Callsign; }
	const CIp &GetIp(void) const                        { return m_Ip; }
	bool HasModule(void) const                          { return m_Callsign.HasModule(); }
	char GetModule(void) const                          { return m_Callsign.GetModule(); }
	bool HasReflectorModule(void) const                 { return m_ReflectorModule != ' '; }
	char GetReflectorModule(void) const                 { return m_ReflectorModule; }

	// set
	void SetModule(char c)                              { m_Callsign.SetModule(c); }
	void SetReflectorModule(char c)                     { m_ReflectorModule = c; }

	// identity
	virtual int GetProtocol(void) const                 { return PROTOCOL_NONE; }
	virtual const char *GetProtocolName(void) const     { return "none"; }
	virtual bool IsNode(void) const                     { return false; }
	virtual bool IsPeer(void) const                     { return false; }
	virtual bool IsDextraDongle(void) const             { return false; }
	virtual void SetDextraDongle(void)                  { }

	// status
	virtual void Alive(void);
	virtual bool IsAlive(void) const                    { return false; }
	virtual bool IsAMaster(void) const                  { return (m_ModuleMastered != ' '); }
	virtual void SetMasterOfModule(char c)              { m_ModuleMastered = c; }
	virtual void NotAMaster(void)                       { m_ModuleMastered = ' '; }
	virtual void Heard(void)                            { m_LastHeardTime = std::time(nullptr); }

	// reporting
	virtual void WriteXml(std::ofstream &);

protected:
	// data
	CCallsign   m_Callsign;
	CIp         m_Ip;

	// linked to
	char        m_ReflectorModule;

	// status
	char        m_ModuleMastered;
	CTimePoint  m_LastKeepaliveTime;
	std::time_t m_ConnectTime;
	std::time_t m_LastHeardTime;
};
