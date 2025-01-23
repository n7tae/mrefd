//
//  Copyright © 2020 Thomas A. Early, N7TAE
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

////////////////////////////////////////////////////////////////////////////////////////
// aliases

// M17 Packets
//all structures must be big endian on the wire, so you'll want htonl (man byteorder 3) and such.
using SM17Lich = struct __attribute__((__packed__)) lich_tag {
	uint8_t  addr_dst[6];
	uint8_t  addr_src[6];
	uint16_t frametype;      //frametype flag field per the M17 spec
	uint8_t  meta_data[14];  //bytes for the meta data, as described in the M17 spec
}; // 6 + 6 + 2 + 14 = 28 bytes

//without SYNC or other parts
using SM17Frame = struct __attribute__((__packed__)) m17_tag {
	uint8_t  magic[4];
	uint16_t streamid;
	SM17Lich lich;
	uint16_t framenumber;
	uint8_t  payload[16];
	uint16_t crc; 	//16 bit CRC
}; // 4 + 2 + 28 + 2 + 16 + 2 = 54 bytes

// includes extra bool (1 byte) for enforcing one-hop policy
using SRefM17Frame = struct __attribute__((__packed__)) peer_tag {
	SM17Frame frame;
	bool relayed;
}; // 4 + 2 + 28 + 2 + 16 + 2 + 1 = 55 bytes

using SInterConnect = struct __attribute__((__packed__)) interconnect_tag {
	uint8_t magic[4];
	uint8_t fromcs[6];
	uint8_t mods[27];
}; // 37 bytes

class CPacket
{
public:
	CPacket() {}
	CPacket(const uint8_t *buf, bool is_internal);
	const CCallsign &GetDestCallsign() const;
	char GetDestModule() const;
	const CCallsign &GetSourceCallsign() const;
	uint16_t GetStreamId() const;
	uint16_t GetFrameType() const;
	uint16_t GetCRC() const;
	void SetCRC(uint16_t crc);
	void SetRelay(bool state);
	bool GetRelay() const;
	std::unique_ptr<CPacket> Duplicate(void) const;
	bool IsFirstPacket() const;
	bool IsLastPacket() const;
	SRefM17Frame &GetFrame();

private:
	CCallsign destination, source;
	SRefM17Frame m17;
};
