//
//  Created by Jean-Luc Deltombe (LX3JL) on 31/01/2016.
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

#include <string>
#include <future>

#include "main.h"
#include "callsign.h"
#include "ip.h"


class CIFileItem
{
public:
	// constructor
	CIFileItem();
	CIFileItem(const CCallsign &cs, const char *mods);
	CIFileItem(const CCallsign &cs, const char *addr, const char *mods, uint16_t port);

	// set
	void SetIP(const char *addr, uint16_t port);

	// compare
	bool HasSameCallsign(const CCallsign &) const;
	bool HasSameIp(const CIp &ip);
	bool HasModuleListed(char) const;
	bool CheckListedModules(const char*) const;

	// get
	const CIp &GetIp(void) const              { return m_Ip; }
	const CCallsign &GetCallsign(void) const  { return m_Callsign; }
	const std::string &GetModules(void) const { return m_Mods; }

	// data
	CCallsign   m_Callsign;
	CIp         m_Ip;
	std::string m_Mods;
	uint16_t    m_Port;
#ifndef NO_DHT
	bool m_Updated;
	std::string m_IPv4, m_IPv6, m_EncryptedMods;
	mutable std::future<size_t> m_Future;
#endif
};
