//
//  ccallsignlistitem.cpp
//  m17ref
//
//  Created by Jean-Luc Deltombe (LX3JL) on 31/01/2016.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020,2022 Thomas A. Early N7TAE
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

#include <string.h>

#include "main.h"
#include "configure.h"
#include "ifileitem.h"
#include "reflector.h"

////////////////////////////////////////////////////////////////////////////////////////
// constructor

CIFileItem::CIFileItem() {}

CIFileItem::CIFileItem(const CCallsign &cs, const char *mods)
{
	m_Callsign.CSIn(cs.GetCS());
	m_Mods.assign(mods);
}

CIFileItem::CIFileItem(const CCallsign &cs, const char *addr, const char *mods, uint16_t port) : CIFileItem(cs, mods)
{
	m_Ip.Initialize(strchr(addr, ':') ? AF_INET6 : AF_INET, port, addr);
}

// set

void CIFileItem::SetIP(const char *addr, uint16_t port)
{
	m_Ip.Initialize(strchr(addr, ':') ? AF_INET6 : AF_INET, port, addr);
}


////////////////////////////////////////////////////////////////////////////////////////
// compare

bool CIFileItem::HasSameCallsign(const CCallsign &callsign) const
{
	return m_Callsign.HasSameCallsign(callsign);
}

bool CIFileItem::HasModuleListed(char module) const
{
	return m_Mods.npos != m_Mods.find(module);
}

bool CIFileItem::HasSameIp(const CIp &ip)
{
	return ip == m_Ip;
}

bool CIFileItem::CheckListedModules(const char *mods) const
{
	if (mods == nullptr)
		return false;

	// make sure every mods character is matched in m_Mods
	const auto count = m_Mods.size();
	bool found[count];
	for (unsigned i=0; i<count; i++)
		found[i] = false;
	for (auto p=mods; *p; p++)
	{
		auto pos = m_Mods.find(*p);
		if (pos == m_Mods.npos)
			return false;
		else
			found[pos] = true;
	}
	for (unsigned i=0; i<count; i++)
	{
		if (! found[i])
			return false;
	}
	return true;
}
