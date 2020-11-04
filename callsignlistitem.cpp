//
//  ccallsignlistitem.cpp
//  m17ref
//
//  Created by Jean-Luc Deltombe (LX3JL) on 31/01/2016.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
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
#include "callsignlistitem.h"

////////////////////////////////////////////////////////////////////////////////////////
// constructor

CCallsignListItem::CCallsignListItem() {}

CCallsignListItem::CCallsignListItem(const CCallsign &callsign, const CIp &ip, const char *modules)
{
	m_Callsign = callsign;
	m_Ip = ip;
	m_Ip.SetPort(M17_PORT);
	m_Mods.clear();
	if ( modules != nullptr )
	{
		if ( modules[0] == '*' )
		{
			for ( char i = 0; i < NB_OF_MODULES; i++ )
			{
				m_Mods.append(1, 'A' + i);
			}
		}
		else
		{
			for (const char *p=modules; *p; p++)
			{
				// duplicates not allowed!
				if (m_Mods.npos == (m_Mods.find(*p)))
				{
					int i = *p - 'A';
					// don't add mods that aren't configured
					if (i >= 0 && i < NB_OF_MODULES)
					{
						m_Mods.append(1, *p);
					}
					else
					{
						std::cerr << "Peer module " << *p << " is not configured!" << std::endl;
					}
				}
				else
				{
					std::cout << "Warning: Module " << *p << " is listed multiple times!" << std::endl;
				}

			}
		}
	}
}

CCallsignListItem::CCallsignListItem(const CCallsign &callsign, const char *url, const char *modules)
{
	CIp ip(strchr(url, ':') ? AF_INET6 : AF_INET, M17_PORT, url);
	CCallsignListItem(callsign, ip, modules);
}

CCallsignListItem::CCallsignListItem(const CCallsignListItem &item)
{
	m_Callsign = item.m_Callsign;
	m_Ip = item.m_Ip;
	m_Mods.assign(item.m_Mods);
}


////////////////////////////////////////////////////////////////////////////////////////
// compare

bool CCallsignListItem::HasSameCallsign(const CCallsign &callsign) const
{
	return m_Callsign.HasSameCallsign(callsign);
}

bool CCallsignListItem::HasSameCallsignWithWildcard(const CCallsign &callsign) const
{
	return m_Callsign.HasSameCallsignWithWildcard(callsign);
}

bool CCallsignListItem::HasModuleListed(char module) const
{
	return m_Mods.npos != m_Mods.find(module);
}

bool CCallsignListItem::HasSameIp(const CIp &ip)
{
	return ip == m_Ip;
}

bool CCallsignListItem::CheckListedModules(const char *mods) const
{
	if (mods == nullptr)
		return false;

	// make sure every mods character is matched in m_Mods
	const auto count = m_Mods.size();
	bool found[count] = { false };
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
