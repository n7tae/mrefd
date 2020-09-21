//
//  Created by Antony Chazapis (SV9OAN) on 25/2/2018.
//  Copyright © 2016 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020 Thomas A. Early N7TAE
//
// ----------------------------------------------------------------------------
//    This file is part of mrefd.
//
//    mrefd is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    mrefd is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#pragma once

#include "peer.h"
#include "m17client.h"

////////////////////////////////////////////////////////////////////////////////////////
// define


////////////////////////////////////////////////////////////////////////////////////////
// class

class CM17Peer : public CPeer
{
public:
	// constructors
	CM17Peer();
	CM17Peer(const CCallsign &, const CIp &, const char *);
	CM17Peer(const CM17Peer &) = delete;

	// status
	bool IsAlive(void) const;

	// identity
	int GetProtocol(void) const                 { return PROTOCOL_M17; }
	const char *GetProtocolName(void) const     { return "M17"; }

	// revision helper
	static int GetProtocolRevision(const CVersion &);
};
