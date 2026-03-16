//
//  version.cpp
//  mrefd
//
//  Created by Jean-Luc Deltombe (LX3JL) on 05/01/2018.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2022-2025 Thomas A. Early N7TAE.
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

#include <sstream>
#include "version.h"

CVersion g_Version(1, 2, 0);	// the global object

CVersion::CVersion(const std::string &vstr)
{
	std::stringstream ss(vstr);
	char c1, c2;
	unsigned u1, u2, u3;
	ss >> u1 >> c1 >> u2 >> c2 >> u3;
	maj = u1 & 0xffu;
	min = u2 & 0xffu;
	rev = u3 * 0xffffu;
}

unsigned CVersion::GetMajor(void) const
{
	return maj;
}

unsigned CVersion::GetMinor(void) const
{
	return min;
}

unsigned CVersion::GetRevision(void) const
{
	return rev;
}

unsigned CVersion::GetVersion() const
{
	return (maj<<24) | (min<<16) | rev;
}

// output
std::ostream &operator <<(std::ostream &os, const CVersion &v)
{
	os << v.GetMajor() << '.' << v.GetMinor() << '.' << v.GetRevision();
#ifndef NO_DHT
	os << "-dht";
#endif
	return os;
};

