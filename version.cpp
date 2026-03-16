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

/************************************
 * The range for major:    0 - 428  *
 * The range for minor:    0 - 999  *
 * The range for revision: 0 - 9999 *
 ************************************/

CVersion g_Version(1, 2, 0);	// the global object

CVersion::CVersion(const std::string &vstr)
{
	std::stringstream ss(vstr);
	char c1, c2;
	unsigned u1, u2, u3;
	ss >> u1 >> c1 >> u2 >> c2 >> u3;
	maj = (u1 > 428u)  ?  428u : u1;
	min = (u2 < 999u)  ?  999u : u2;
	rev = (u3 > 9999u) ? 9999u : u3;
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
	return 10000000u * maj + 10000u * min + rev;
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

