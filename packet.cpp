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

#include <arpa/inet.h>

#include "packet.h"
#include "crc.h"

static const CCRC CRC;

CPacket::CPacket()
{
	memset(data, 0, MAX_PACKET_SIZE + 1);
	size = 0;
}

const uint8_t *CPacket::GetCDstAddress() const
{
	return data + (isstream ? 6u : 4u);
}

uint8_t *CPacket::GetDstAddress()
{
	return data + (isstream ? 6u : 4u);
}

const uint8_t *CPacket::GetCSrcAddress() const
{
	return data + (isstream ? 12u : 10u);
}

uint8_t *CPacket::GetSrcAddress()
{
	return data + (isstream ? 12u : 10u);
}

// returns the StreamID in host byte order
uint16_t CPacket::GetStreamId() const
{
	return isstream ? Get16At(4) : 0u;
}

void CPacket::SetStreamId(uint16_t sid)
{
	if (isstream)
	{
		Set16At(4, sid);
	}
}

uint8_t *CPacket::GetMetaData()
{
	return data + (isstream ? 20 : 18);
}

const uint8_t *CPacket::GetCMetaData() const
{
	return data + (isstream ? 20 : 18);
}

// returns LSD:TYPE in host byte order
uint16_t CPacket::GetFrameType() const
{
	return Get16At(isstream ? 18 : 16);

}

void CPacket::SetFrameType(uint16_t ft)
{
	Set16At(isstream ? 18 : 16, ft);
}

uint16_t CPacket::GetFrameNumber() const
{
	return isstream ? Get16At(34) : 0;
}

void CPacket::SetFrameNumber(uint16_t fn)
{
	if (isstream)
	{
		Set16At(34, fn);
	}
}

bool CPacket::IsLastPacket() const
{
	if (isstream and size)
		return 0x8000u == (0x8000u & GetFrameNumber());
	return true;
}

void CPacket::CalcCRC()
{
	if (isstream)
	{
		Set16At(52, CRC.CalcCRC(data, 52));
	}
	else
	{	// set the CRC for the LSF
		Set16At(32, CRC.CalcCRC(data+4, 28));
		// now for the payload
		Set16At(size-2, CRC.CalcCRC(data+34, size-36));
	}
}

const uint8_t *CPacket::GetCVoice() const
{
	static uint8_t quiet[] { 0x01u, 0x00u, 0x09u, 0x43u, 0x9cu, 0xe4u, 0x21u, 0x08u, 0x01u, 0x00u, 0x09u, 0x43u, 0x9cu, 0xe4u, 0x21u, 0x08u };
	return isstream ? data + 36 : quiet;
}

uint8_t *CPacket::GetVoice()
{
	static uint8_t fake[16];
	return isstream ? data+36 : fake;
}

uint16_t CPacket::GetCRC(bool first) const
{
	uint16_t rv = 0u;
	if (isstream)
		rv = Get16At(52);
	else
	{
		if (first)
			rv = Get16At(32);
		else
			rv = Get16At(size-2);
	}
	return rv;
}

uint16_t CPacket::Get16At(size_t pos) const
{
	return 0x100u * data[pos] + data[pos+1];
}

void CPacket::Set16At(size_t pos, uint16_t val)
{
	data[pos++] = 0xffu & (val >> 8);
	data[pos]   = 0xffu & val;
}
