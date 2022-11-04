//
//  Created by Jean-Luc Deltombe (LX3JL) on 30/12/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020 Thomas A. Early, N7TAE
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

#include <unordered_map>

#include "main.h"
#include "ifileitem.h"

////////////////////////////////////////////////////////////////////////////////////////
// class

class CIFileMap
{
public:
	// constructor
	CIFileMap();

	// destructor
	virtual ~CIFileMap() {}

	// locks
	void Lock(void)   const { m_Mutex.lock(); }
	void Unlock(void) const { m_Mutex.unlock(); }

	// file io
	virtual bool LoadFromFile(const char *);
	bool ReloadFromFile(void);
	bool NeedReload(void);

	// compare
	bool IsCallsignListed(const CCallsign &, const char) const;
	bool IsCallsignListed(const CCallsign &, const CIp &ip, const char*) const;

	// pass-thru
	bool empty() const { return m_InterlinkMap.empty(); }
	std::unordered_map<std::string, CIFileItem>::iterator begin() { return m_InterlinkMap.begin(); }
	std::unordered_map<std::string, CIFileItem>::iterator end()   { return m_InterlinkMap.end(); }
	std::unordered_map<std::string, CIFileItem>::const_iterator cbegin() { return m_InterlinkMap.cbegin(); }
	std::unordered_map<std::string, CIFileItem>::const_iterator cend()   { return m_InterlinkMap.cend(); }

	// find
	CIFileItem *FindMapItem(const std::string &);

protected:
	bool GetLastModTime(time_t *);
	char *TrimWhiteSpaces(char *);
	char *ToUpper(char *str);

	// data
	mutable std::mutex m_Mutex;
	const char *m_Filename;
	time_t m_LastModTime;
	std::unordered_map<std::string, CIFileItem> m_InterlinkMap;
};
