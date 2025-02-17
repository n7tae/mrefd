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
#include <vector>

#include "callsign.h"

class CPacket
{
public:
	CPacket() {}
	~CPacket() { data.clear(); }
	void Fill(const uint8_t *buf, size_t size, bool bis);
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
	const uint8_t *GetPData() const { return data.data(); }
	size_t GetSize() const { return data.size(); }

private:
	bool isstream;
	std::vector<uint8_t> data;
};
