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

#include <random>
#include <chrono>
#include <cstdint>

class CNewStreamID
{
public:
	CNewStreamID() { srandom(std::chrono::system_clock::now().time_since_epoch().count()); }

	uint16_t Make()
	{
		uint16_t rv = 0;
		while (0 == rv)
			rv = 0xffffU & random();
		return rv;
	}
};
