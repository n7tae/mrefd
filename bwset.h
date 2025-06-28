//
//  Copyright © 2020 Thomas A. Early, N7TAE
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

#include <unordered_set>
#include <string>
#include <mutex>

////////////////////////////////////////////////////////////////////////////////////////
// class

class CBWSet
{
public:
	// constructor
	CBWSet();

	// file io
	bool LoadFromFile(const char *);
	bool ReloadFromFile(void);
	bool NeedReload(void);

	// compare
	bool IsEmptyOrMatched(const std::string &) const;
	bool IsMatched(const std::string &) const;

protected:
	bool matched(const std::string &) const;
	bool GetLastModTime(time_t *);
	char *TrimWhiteSpaces(char *);
	char *ToUpper(char *str);

	// data
	mutable std::mutex  m_Mutex;
	const char *m_Filename;
	time_t      m_LastModTime;
	std::unordered_set<std::string> m_Callsigns;
};
