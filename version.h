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

#pragma once

#include <cstdint>
#include <iostream>

class CVersion
{
public:
	// constructor
	CVersion() = delete;
	CVersion(const std::string &vstr);
	CVersion(uint16_t a, uint16_t b, uint16_t c) : major(a), minor(b), revision(c) {}
	~CVersion() {}

	// get
	unsigned GetVersion(void) const;

	// output
	friend std::ostream &operator <<(std::ostream &os, const CVersion &v);


private:
	void checkInput(uint16_t &val, const std::string &label, unsigned proposed, unsigned max);

	// data
	uint16_t major, minor, revision;
};
