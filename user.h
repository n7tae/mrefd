//
//  cuser.h
//  m17ref
//
//  Created by Jean-Luc Deltombe (LX3JL) on 13/11/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
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

#include <ctime>

#include "callsign.h"

////////////////////////////////////////////////////////////////////////////////////////

class CUser
{
public:
	// constructor
	CUser();
	CUser(const CCallsign &, const CCallsign &, const CCallsign &);
	CUser(const CUser &);

	// get data
	std::string GetSource(void)        const { return m_Source.GetCS(); }
	std::string GetDestination(void)   const { return m_Destination.GetCS(); }
	std::string GetReflector(void)     const { return m_Reflector.GetCS(); }
	std::time_t GetLastHeardTime(void) const { return m_LastHeardTime;}

	// operation
	void HeardNow(void)     { m_LastHeardTime = std::time(nullptr); }

	// operators
	bool operator ==(const CUser &) const;
	bool operator <(const CUser &) const;

	// reporting
	void WriteXml(std::ofstream &);

protected:
	// data
	CCallsign   m_Source;
	CCallsign   m_Destination;
	CCallsign   m_Reflector;
	std::time_t m_LastHeardTime;
};
