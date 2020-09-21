//
//  cnotificationqueue.h
//  M17Refd
//
//  Created by Jean-Luc on 05/12/2015.
//  Copyright © 2015 Jean-Luc. All rights reserved.
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

#include "notification.h"


////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
// class

class CNotificationQueue
{
public:
	// constructor
	CNotificationQueue() {}

	// destructor
	~CNotificationQueue() {}

	// lock
	void Lock()                   { m_Mutex.lock(); }
	void Unlock()                 { m_Mutex.unlock(); }

	// pass thru
	CNotification front()         { return queue.front(); }
	void pop()                    { queue.pop(); }
	void push(CNotification note) { queue.push(note); }
	bool empty() const            { return queue.empty(); }

protected:
	// data
	std::mutex  m_Mutex;
	std::queue<CNotification> queue;
};
