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
#include "peermapitem.h"
#include "reflector.h"

////////////////////////////////////////////////////////////////////////////////////////
// constructor

CPeerMapItem::CPeerMapItem() {}

CPeerMapItem::CPeerMapItem(const CCallsign &callsign, const CIp &ip, const char *modules)
{
	m_Callsign.CSIn(callsign.GetCS());
	m_Ip = ip;
	m_Ip.SetPort(g_CFG.GetPort());
	m_Mods.clear();
	if ( modules != nullptr )
	{
		if ( modules[0] == '*' )
		{
			for (char c='A'; c<='Z'; c++)
			{
				if (g_CFG.IsValidModule(c))
				m_Mods.append(1, c);
			}
		}
		else
		{
			for (const char *p=modules; *p; p++)
			{
				// duplicates not allowed!
				if (m_Mods.npos == (m_Mods.find(*p)))
				{
					// don't add mods that aren't configured
					if (g_CFG.IsValidModule(*p))
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

CPeerMapItem::CPeerMapItem(const CCallsign &callsign, const char *url, const char *modules)
{
	m_Callsign.CSIn(callsign.GetCS());
	m_Ip = CIp(strchr(url, ':') ? AF_INET6 : AF_INET, g_CFG.GetPort(), url);
	m_Mods.assign(modules);
}

CPeerMapItem::CPeerMapItem(const CPeerMapItem &item)
{
	m_Callsign.CSIn(item.m_Callsign.GetCS());
	m_Ip = item.m_Ip;
	m_Mods.assign(item.m_Mods);
}

void CPeerMapItem::operator=(const CPeerMapItem &rhs)
{
	m_Callsign.CSIn(rhs.m_Callsign.GetCS());
	m_Ip = rhs.m_Ip;
	m_Mods.assign(rhs.m_Mods);
}

////////////////////////////////////////////////////////////////////////////////////////
// compare

bool CPeerMapItem::HasSameCallsign(const CCallsign &callsign) const
{
	return m_Callsign.HasSameCallsign(callsign);
}

bool CPeerMapItem::HasModuleListed(char module) const
{
	return m_Mods.npos != m_Mods.find(module);
}

bool CPeerMapItem::HasSameIp(const CIp &ip)
{
	return ip == m_Ip;
}

bool CPeerMapItem::CheckListedModules(const char *mods) const
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
