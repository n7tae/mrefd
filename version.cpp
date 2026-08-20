/*
	A usable Version class
	Copyright (C) 2026 Thomas A. Early, N7TAE

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <sstream>

#include "version.h"

/************************************
 * The range for major:    0 - 428  *
 * The range for minor:    0 - 999  *
 * The range for revision: 0 - 9999 *
 ************************************/

CVersion::CVersion(const std::string &vstr)
{
	std::stringstream ss(vstr);
	char c1, c2;
	unsigned u1, u2, u3;
	ss >> u1 >> c1 >> u2 >> c2 >> u3;

	checkInput(major, "major", u1, 428);
	checkInput(minor, "minor", u2, 999);
	checkInput(revision, "revision", u3, 9999);
}

unsigned CVersion::GetVersion() const
{
	return 10000000u * major + 10000u * minor + revision;
}

void CVersion::checkInput(uint16_t &val, const std::string &label, unsigned proposed, unsigned maximum)
{
	if (proposed > maximum)
	{
		std::cout << "CVersion WARNING: Value for " << label << ", " << proposed << ", is too large. Resetting to " << maximum << std::endl;
		proposed = maximum;
	}
	val = proposed;
}

// output
std::ostream &operator <<(std::ostream &os, const CVersion &v)
{
	os << v.major << '.' << v.minor << '.' << v.revision;
#ifndef NO_DHT
	os << "-dht";
#endif
	return os;
};
