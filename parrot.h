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
#include "client.h"
#include "ip.h"
#include "timer.h"

enum class EParrotState { record, play, done };

class CParrot
{
public:
	CParrot(const uint8_t *src_addr, SPClient spc, uint16_t ft) : src(src_addr), client(spc), frameType(ft), state(EParrotState::record) {}
	virtual ~CParrot() {}
	virtual void Add(const CPacket &pack) = 0;
	virtual bool IsExpired() const = 0;
	virtual void Play() = 0;
	virtual bool IsStream() const = 0;
	EParrotState GetState() const { return state; }
	const CCallsign &GetSRC() const { return src; }
	void Quit() { if (fut.valid()) fut.get(); }

protected:
	const CCallsign src;
	SPClient client;
	const uint16_t frameType;
	std::atomic<EParrotState> state;
	std::future<void> fut;
};

class CStreamParrot : public CParrot
{
public:
	CStreamParrot(const uint8_t *src_addr, SPClient spc, uint16_t ft) : CParrot(src_addr, spc, ft), is3200(0u == (0x2u & ft)) {}
	void Add(const CPacket &pack);
	void Play();
	bool IsExpired() const { return lastHeard.Time() > STREAM_TIMEOUT; }
	size_t GetSize() const { return size; }
	bool IsStream() const { return true; }

private:
	std::vector<std::vector<uint8_t>> data;
	const bool is3200;
	CTimer lastHeard;
	size_t size;

	void playThread();
};

class CPacketParrot : public CParrot
{
public:
	CPacketParrot(const uint8_t *src_addr, SPClient spc, uint16_t ft) : CParrot(src_addr, spc, ft) {}
	void Add(const CPacket &pack);
	bool IsExpired() const { return false; }
	void Play();
	bool IsStream() const { return false; }

private:
	CPacket packet;

	void returnPacket();
};
