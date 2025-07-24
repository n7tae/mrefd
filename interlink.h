//
//  Created by Jean-Luc Deltombe (LX3JL) on 31/01/2016.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2025 Thomas A. Early N7TAE
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

#include <string>

#include "callsign.h"
#include "refmods.h"
#include "ip.h"

class CInterlink
{
public:
	// constructor
	CInterlink() = delete;
#ifndef NO_DHT
	CInterlink(const std::string &cs, const std::string &mods);
#endif
	CInterlink(const std::string &cs, const std::string &mods, const std::string &addr, uint16_t port, bool islegacy);

	void UpdateItem(const std::string &targetmods, const std::string &emods, const std::string &ipv4, const std::string &ipv6, const std::string &url, uint16_t port, bool islegacy);

	// get
	const CIp &GetIp(void) const              { return m_Ip; }
	const CCallsign &GetCallsign(void) const  { return m_Callsign; }
	bool IsNotLegacy(void) const              { return m_IsNotLegacy; }
	bool IsUsingDHT(void) const               { return m_UsingDHT; }
	const std::string &GetIPv4(void) const    { return m_IPv4; }
	const std::string &GetIPv6(void) const    { return m_IPv6; }
	const std::string &GetDashUrl(void) const { return m_dashUrl; }
	const std::string &GetReqMods(void) const { return m_reqMods; }

private:
	// data
	const bool  m_UsingDHT;
	const std::string m_reqMods;
	const CCallsign   m_Callsign;
	CIp         m_Ip;
	std::string m_dashUrl;
	uint16_t    m_Port;
	bool        m_IsNotLegacy;

#ifndef NO_DHT
	std::string m_IPv4, m_IPv6;
#endif
};
