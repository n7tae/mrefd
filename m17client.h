#pragma once

//  Copyright © 2015 Thomas A. Early, N7TAE
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

#include "client.h"

////////////////////////////////////////////////////////////////////////////////////////
// define


////////////////////////////////////////////////////////////////////////////////////////
// class

class CM17Client : public CClient
{
public:
	// constructors
	CM17Client();
	CM17Client(const CCallsign &, const CIp &, char);
	CM17Client(const CM17Client &);

	// destructor
	virtual ~CM17Client() {};

	// identity
	int GetProtocol(void) const                 { return PROTOCOL_M17; }
	const char *GetProtocolName(void) const     { return "M17"; }
	bool IsNode(void) const                     { return true; }

	// status
	bool IsAlive(void) const;
};

////////////////////////////////////////////////////////////////////////////////////////
