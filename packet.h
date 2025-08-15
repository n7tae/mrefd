//
//  Copyright © 2020-2025 Thomas A. Early, N7TAE
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

#include <cstdint>
#include <string.h>
#include <memory>

#include "defines.h"

class CPacket
{
public:
	CPacket();
	// get pointers to different parts
	      uint8_t *GetData()        { return data; }
	const uint8_t *GetCData() const { return data; }
	      uint8_t *GetDstAddress();
	const uint8_t *GetCDstAddress() const;
	      uint8_t *GetSrcAddress();
	const uint8_t *GetCSrcAddress() const;
	      uint8_t *GetMetaData();
	const uint8_t *GetCMetaData() const;
	      uint8_t *GetVoice();
	const uint8_t *GetCVoice() const;

	// get various 16 bit value in host byte order
	uint16_t GetStreamId()    const;
	uint16_t GetFrameType()   const;
	uint16_t GetFrameNumber() const;
	uint16_t GetCRC(bool first = true) const;

	// set 16 bit values in network byte order
	void SetStreamId(uint16_t sid);
	void SetFrameType(uint16_t ft);
	void SetFrameNumber(uint16_t fn);

	// get the state data
	size_t          GetSize() const { return size; }
	EClientType GetFromType() const { return type; }
	bool       IsStreamData() const { return isstream; }
	bool       IsPacketData() const { return not isstream; }
	bool       IsLastPacket() const;

	// set state data 
	void SetSize(size_t n) { size = n; }
	void SetFromType(EClientType f) { type = f; }
	void SetType(bool iss) { isstream = iss; }
	void Initialize(size_t n, bool iss) { size = n; isstream = iss; }

	// calculate and set CRC value(s)
	void CalcCRC();

private:
	uint16_t Get16At(size_t pos) const;
	void Set16At(size_t pos, uint16_t val);
	
	bool isstream;
	EClientType type;
	size_t size;
	uint8_t data[MAX_PACKET_SIZE];
};
