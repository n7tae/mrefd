/*

                mrefd - an M17-only reflector
            Copyright (C) 2025 Thomas A. Early N7TAE

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/

#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

#include "packet.h"

template <class T>
class CSafePacketQueue
{
public:
 	CSafePacketQueue(void) : q() , m() , c() {}

	virtual ~CSafePacketQueue(void) {}

	void Push(T &t)
	{
		std::lock_guard<std::mutex> lock(m);
		q.push(std::move(t));
		c.notify_one();
	}

	T Pop(void)
	{
		std::unique_lock<std::mutex> lock(m);
		if (q.empty())
			return nullptr;
		else
		{
			T val = std::move(q.front());
			q.pop();
			return val;
		}
	}

	// If the queue is empty, wait until an element is available.
	T PopWait(void)
	{
		std::unique_lock<std::mutex> lock(m);
		while(q.empty())
		{
			// release lock as long as the wait and reacquire it afterwards.
			c.wait(lock);
		}
		T val = std::move(q.front());
		q.pop();
		return val;
	}

	// wait for some time, or until an element is available.
	T PopWaitFor(int ms)
	{
		std::unique_lock<std::mutex> lock(m);
		T val;
		if (c.wait_for(lock, std::chrono::milliseconds(ms), [this] { return not q.empty(); }))
		{
			val = std::move(q.front());
			q.pop();
		}
		return val;
	}

	bool IsEmpty(void)
	{
		std::unique_lock<std::mutex> lock(m);
		return q.empty();
	}

private:
	std::queue<T> q;
	mutable std::mutex m;
	std::condition_variable c;
};

using CPacketQueue = CSafePacketQueue<std::unique_ptr<CPacket>>;
