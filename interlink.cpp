//
//  ccallsignlistitem.cpp
//  mrefd
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

#include <string.h>

#include "configure.h"
#include "interlink.h"
#include "reflector.h"

extern CConfigure g_CFG;

////////////////////////////////////////////////////////////////////////////////////////
// constructor

#ifndef NO_DHT
CInterlink::CInterlink(const std::string &cs, const std::string &mods) : m_UsingDHT(true), m_reqMods(mods)
{
	m_Updated = false;
	m_Callsign.CSIn(cs);
}
#endif

CInterlink::CInterlink(const std::string &cs, const std::string &mods, const std::string &addr, uint16_t port, bool islegacy) : m_UsingDHT(false), m_reqMods(mods)
{
	m_Callsign.CSIn(cs);
	if (addr.npos == addr.find(':'))
		UpdateItem("", "", addr, "", port, islegacy);
	else
		UpdateItem("", "", "", addr, port, islegacy);
}

////////////////////////////////////////////////////////////////////////////////////////
// compare

void CInterlink::UpdateItem(const std::string &targetmods, const std::string &emods, const std::string &ipv4, const std::string &ipv6, uint16_t port, bool islegacy)
{
	m_Updated = false;
	// if we know what the target's modules are, then we can make sure the requested modules are suppored
	if (targetmods.size() > 0)
	{
		bool quit = false;
		for (const auto m : m_reqMods)
		{
			if (std::string::npos == targetmods.find(m))
			{
				std::cerr << "ERROR: " << m_Callsign.GetCS() << " doesn't have a module '" << m << "'" << std::endl;
				quit = true;
			}
		}
		if (quit) return;
	}
	m_IsNotLegacy = not islegacy;	// this is the gatekeeper for sending packets to a reflector

	// checkout the modules first
	CReflMods tmprm(m_reqMods, emods);
	if (tmprm.GetSize())
	{
		if (tmprm.IsIn(g_CFG.GetRefMods(), m_UsingDHT))
		{
			if (not (tmprm == m_refmods))
			{
				m_refmods = tmprm;
				m_Updated = true;
			}
		}
		else
		{
			std::cout << "ERROR: There is a mismatched module specifed for " << m_Callsign.GetCS() << std::endl;
			return;
		}
	}
	else
	{
		std::cout << "ERROR: Can't find any modules for " << m_Callsign.GetCS() << std::endl;
		return;
	}
	// now the other stuff
	if (m_IPv4.compare(ipv4))
	{
		m_IPv4.assign(ipv4);
		m_Updated = true;
	}
	if (m_IPv6.compare(ipv6))
	{
		m_IPv6.assign(ipv6);
		m_Updated = true;
	}
	if (m_Port != port)
	{
		m_Port = port;
		m_Updated = true;
	}
	if (g_CFG.GetIPv6BindAddr().empty())
	{
		if (m_IPv4.empty())
		{
			std::cout << "ERROR: " << m_Callsign.GetCS() << " doesn't have an IPv4 address" << std::endl;
			m_Updated = false;
			return;
		}
		else
			m_Ip.Initialize(AF_INET, m_Port, m_IPv4.c_str());
	}
	else
	{
		if (m_IPv6.empty())
		{
			std::cout << "ERROR: " << m_Callsign.GetCS() << " doesn't have an IPv6 address" << std::endl;
			m_Updated = false;
			return;
		}
		else
			m_Ip.Initialize(AF_INET6, m_Port, m_IPv4.c_str());
	}
}
