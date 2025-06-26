//
//  Copyright © 2025 Thomas A. Early, N7TAE
//
// ----------------------------------------------------------------------------
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
#include <map>

#include "defines.h"

using CMMMap = std::map<char, EModuleMode>;

class CReflMods
{
public:
	// ctors
	CReflMods() {}
	CReflMods(const std::string &s, const std::string &e);
	CReflMods &operator=(const CReflMods &item);
	~CReflMods() { Clear(); }
	// add modules to this map
	void Parse(const std::string &s, const std::string &e);
	// utilities
	size_t GetSize() const { return mmMap.size(); }
	bool IsIn(const CReflMods &rm, bool checkmodes) const;
	void Clear() { mmMap.clear(); mods.erase(); emods.erase(); }
	// get data from the map
	const std::string &GetModules() const { return mods; };
	const std::string &GetEModules() const { return emods; };
	bool GetMode(char, EModuleMode &) const;
	const char *GetModeName(EModuleMode mm) const;
	const CMMMap &GetMap() const { return mmMap; }
	bool operator==(const CReflMods &mm) const { return mods == mm.GetModules() and emods == GetEModules(); }

private:
	// the data
	std::string mods, emods;
	CMMMap mmMap;
};
