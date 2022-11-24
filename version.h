//
//  version.h
//  m17ref
//
//  Created by Jean-Luc Deltombe (LX3JL) on 05/01/2018.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2022 Thomas A. Early N7TAE.
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

#include <cstdint>
#include <iostream>

class CVersion
{
public:
	// constructor
	CVersion(uint8_t maj, uint8_t min, uint8_t rev);

	// get
	uint8_t GetMajor(void) const;
	uint8_t GetMinor(void) const;
	uint8_t GetRevision(void) const;
	int     GetVersion(void) const;

	// comparaison operators
	bool operator ==(const CVersion &v) const;
	bool operator !=(const CVersion &v) const;
	bool operator >=(const CVersion &v) const;
	bool operator <=(const CVersion &v) const;
	bool operator  >(const CVersion &v) const;
	bool operator  <(const CVersion &v) const;

	// output
	friend std::ostream &operator <<(std::ostream &os, const CVersion &v);


protected:
	// data
	const int version;
};
