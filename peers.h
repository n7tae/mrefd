//
//  cpeers.h
//  M17Refd
//
//  Created by Jean-Luc Deltombe (LX3JL) on 10/12/2016.
//  Copyright © 2016 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020 Thomas A. Early, N7TAE
//
// ----------------------------------------------------------------------------
//    This file is part of M17Refd.
//
//    M17Refd is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    M17Refd is distributed in the hope that it will be useful,
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
	void Lock(void)   { m_Mutex.lock(); }
	void Unlock(void) { m_Mutex.unlock(); }

	// manage peers
	int  GetSize(void) const { return (int)m_Peers.size(); }
	void AddPeer(std::shared_ptr<CPeer>);
	void RemovePeer(std::shared_ptr<CPeer>);

	// pass-through
	std::list<std::shared_ptr<CPeer>>::iterator begin()              { return m_Peers.begin(); }
	std::list<std::shared_ptr<CPeer>>::iterator end()                { return m_Peers.end(); }
	std::list<std::shared_ptr<CPeer>>::const_iterator cbegin() const { return m_Peers.cbegin(); }
	std::list<std::shared_ptr<CPeer>>::const_iterator cend() const   { return m_Peers.cend(); }

	// find peers
	std::shared_ptr<CPeer> FindPeer(const CIp &);
	std::shared_ptr<CPeer> FindPeer(const CCallsign &, const CIp &);
	std::shared_ptr<CPeer> FindPeer(const CCallsign &);

	// iterate on peers
	std::shared_ptr<CPeer> FindNextPeer(std::list<std::shared_ptr<CPeer>>::iterator &);

protected:
	// data
	std::mutex         m_Mutex;
	std::list<std::shared_ptr<CPeer>> m_Peers;
};
