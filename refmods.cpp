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

#include <iostream>
#include <sstream>
#include <string>
#include <string.h>

#include "refmods.h"

CReflMods::CReflMods(const std::string &m, const std::string &e)
{
	Parse(m, e);
}

CReflMods &CReflMods::operator=(const CReflMods &item)
{
	mmMap.clear();
	mmMap.insert(item.mmMap.cbegin(), item.mmMap.cend());
	mods.assign(item.mods);
	emods.assign(item.emods);
	return *this;
}

const char *CReflMods::GetModeName(EModuleMode mm) const
{
	return (EModuleMode::normal == mm) ? "not encrypted" : "encrypted";
}

void CReflMods::Parse(const std::string &s, const std::string &e)
{
	mods.clear();
	emods.clear();
	std::string ein;
	for (auto m : e)
	{
		if (isalpha(m))
		{
			if (islower(m))
				m = toupper(m);
			ein.append(1, m);
		}
	}
	for (auto m : s)
	{
		if (isalpha(m))
		{
			if (islower(m))
				m = toupper(m);
			if (mmMap.end() == mmMap.find(m))
			{
				mods.append(1, m);
				if (ein.npos == ein.find(m))
				{
					mmMap[m] = EModuleMode::normal;
				}
				else
				{
					emods.append(1, m);
					mmMap[m] = EModuleMode::encrypted;
				}
			}
		}
	}
}

// this is only used in the Cinterlink::UpdateItem routine to make sure requested modules
// match rm, which is the configurtion of this reflector
bool CReflMods::IsIn(const CReflMods &rm, bool checkmodes, const std::string &cs) const
{
	bool rv = true;
	// check each one of the modules of the interlink request
	for (const auto item : mmMap)
	{
		EModuleMode mm;
		if (rm.GetMode(item.first, mm)) // get the module of this refector
		{
			std::cerr << "ERROR: for enterlink item " << cs << ", module '" << item.first << "' is not configured on this reflector!" << std::endl;
			rv = false;
		}
		if (checkmodes)
		{
			if (item.second != mm)
			{
				std::cerr << "ERROR: for interlink item " << cs << ", module '" << item.first << "' it's mode is " << GetModeName(item.second) << " and is incompatible with this reflector, which is " << GetModeName(mm) << std::endl;
				rv = false;
			}
		}
	}
	return rv;
}

bool CReflMods::GetMode(char c, EModuleMode &mm) const
{
	const auto item = mmMap.find(c);
	if (mmMap.cend() == item)
		return true;
	else
	{
		mm = item->second;
	}
	return false;
}
