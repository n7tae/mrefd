//
//  cpeers.h
//  mrefd
//
//  Created by Jean-Luc Deltombe (LX3JL) on 10/12/2016.
//  Copyright © 2016 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020 Thomas A. Early, N7TAE
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

#include "peer.h"


////////////////////////////////////////////////////////////////////////////////////////
// define


////////////////////////////////////////////////////////////////////////////////////////
// class

class CPeers
{
public:
	// constructors
	CPeers();

	// destructors
	virtual ~CPeers();

	// locks

	// manage peers
	int  GetSize(void) const { return (int)m_Peers.size(); }
	void AddPeer(SPPeer);
	void RemovePeer(SPPeer);

	// pass-through
	std::list<SPPeer>::iterator Begin() { return m_Peers.begin(); }

	// find peers
	SPPeer FindPeer(const CIp &);
	SPPeer FindPeer(const CCallsign &);

	// iterate on peers
	SPPeer FindNextPeer(std::list<SPPeer>::iterator &);

protected:
	void Insert(SPPeer peer);
	// data
	std::mutex         m_Mutex;
	std::list<SPPeer> m_Peers;
};
