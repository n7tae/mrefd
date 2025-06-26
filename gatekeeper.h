//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//
//  Copyright © 2022 Thomas A. Early, N7TAE
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

#pragma once

#include "callsign.h"
#include "ip.h"
#include "bwset.h"
#include "base.h"

////////////////////////////////////////////////////////////////////////////////////////
// class

class CGateKeeper : public CBase
{
public:
	// constructor
	CGateKeeper();

	// destructor
	~CGateKeeper();

	// init & clode
	bool Init(void);
	void Close(void);

	// authorizations
	bool MayLink(const CCallsign &, const CIp &) const;
	bool MayTransmit(const CCallsign &, const CIp &) const;

protected:
	// thread
	void Thread();

	// operation helpers
	bool IsNodeListedOk(const CCallsign &) const;
	bool IsPeerListedOk(const std::string &, const CIp &) const;

	// data
	CBWSet   m_NodeWhiteSet;
	CBWSet   m_NodeBlackSet;

	// thread
	std::atomic<bool> keep_running;
	std::future<void> m_Future;
};
