//
//  Created by Jean-Luc Deltombe (LX3JL) on 02/11/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
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

#include <queue>
#include <mutex>

#include "packet.h"
#include "client.h"

////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
// CPacketQueue

class CClient;

class CPacketQueue
{
public:
	// destructor
	virtual ~CPacketQueue() {}

	// lock
	void Lock()   { m_Mutex.lock(); }
	void Unlock() { m_Mutex.unlock(); }

	// pass thru
	void pop()                                  { queue.pop(); }
	bool empty() const                          { return queue.empty(); }
	std::unique_ptr<CPacket> front()            { return std::move(queue.front()); }
	void push(std::unique_ptr<CPacket> &packet) { queue.push(std::move(packet)); }

protected:
	// status
	bool        m_bOpen;
	uint16_t    m_uiStreamId;
	std::mutex  m_Mutex;

	// owner
	CClient                             *m_Client;
	std::queue<std::unique_ptr<CPacket>> queue;
};
