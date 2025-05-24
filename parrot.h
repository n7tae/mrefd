//
//  Copyright © 2025 Thomas A. Early, N7TAE
//
// ----------------------------------------------------------------------------
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

#include <vector>
#include <cstdint>
#include <atomic>
#include <future>

#include "callsign.h"
#include "ip.h"
#include "timer.h"

class CParrot
{
public:
	CParrot(const uint8_t *src_addr, const CIp &rip, bool isvoiceonly) : src(src_addr), ip(rip), is3200(isvoiceonly), isDone(false), size(0u) {}
	void Add(const uint8_t *v);
	void Play();
	bool IsPlaying() const { return fut.valid(); }
	bool IsDone();
	bool IsExpired() const { return lastHeard.Time() > STREAM_TIMEOUT; }
	size_t GetSize() const { return size; }
	const CCallsign &GetSRC() const { return src; }

private:
	const CCallsign src;
	const CIp &ip;
	std::vector<std::vector<uint8_t>> data;
	const bool is3200;
	std::atomic_bool isDone;
	size_t size;
	std::future<void> fut;
	CTimer lastHeard;

	void playThread();
};
