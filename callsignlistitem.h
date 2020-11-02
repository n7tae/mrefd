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

#include "main.h"
#include "callsign.h"
#include "ip.h"

////////////////////////////////////////////////////////////////////////////////////////
// define

#define URL_MAXLEN          256

////////////////////////////////////////////////////////////////////////////////////////
// class

class CCallsignListItem
{
public:
	// constructor
	CCallsignListItem();
	CCallsignListItem(const CCallsign &, const CIp &, const char *);
	CCallsignListItem(const CCallsign &, const char *, const char *);
	CCallsignListItem(const CCallsignListItem &);

	// destructor
	virtual ~CCallsignListItem() {}

	// compare
	bool HasSameCallsign(const CCallsign &) const;
	bool HasSameCallsignWithWildcard(const CCallsign &) const;
	bool HasSameIp(const CIp &ip);
	bool HasModuleListed(char) const;
	bool CheckListedModules(const char*) const;

	// get
	const CCallsign &GetCallsign(void) const { return m_Callsign; }
	const CIp &GetIp(void) const             { return m_Ip; }
	const std::string &GetModules(void)      { return m_Mods; }

protected:
	// data
	CCallsign   m_Callsign;
	CIp         m_Ip;
	std::string m_Mods;
};
