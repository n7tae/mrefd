//
//  Copyright © 2020-2025 Thomas A. Early, N7TAE
//
// ----------------------------------------------------------------------------
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

#pragma once

#include <cstdint>
#include <string.h>
#include <memory>

#include "callsign.h"

#define MAX_PACKET_SIZE 859

class CPacket
{
public:
	CPacket();
	void SetSize(size_t s, bool iss);
	const uint8_t *GetCDstAddress() const;
	const uint8_t *GetCSrcAddress() const;
	uint8_t *GetDstAddress();
	uint16_t GetStreamId() const;
	uint16_t GetFrameType() const;
	void SetRelay();
	void ClearRelay();
	bool IsRelaySet() const;
	uint16_t GetFrameNumber();
	bool IsStreamPacket() const { return isstream; }
	void CalcCRC();
	uint8_t *GetData() { return data; }
	const uint8_t *GetCData() const { return data; }
	size_t GetSize() const { return size; }

private:
	bool isstream;
	size_t size;
	uint8_t data[MAX_PACKET_SIZE+1];
};
