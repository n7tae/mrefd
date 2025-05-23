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

void CPacket::Initialize(size_t n, bool bis)
{
	size = n;
	isstream = bis;
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
	return isstream ? 0x100u * data[4] + data[5] : 0u;
}

void CPacket::SetStreamId(uint16_t sid)
{
	if (isstream)
	{
		data[4] = (sid >> 8) & 0xffu;
		data[5] = sid & 0xffu;
	}
}

// returns LSD:TYPE in host byte order
uint16_t CPacket::GetFrameType() const
{
	if (isstream)
		return 0x100u * data[18] + data[19];

	return 0x100u * data[16] + data[17];
}

void CPacket::SetFrameType(uint16_t ft)
{
	int offset = isstream ? 18 : 16;
	data[offset] = (ft >> 8) & 0xffu;
	data[offset+1] = ft & 0xffu;
}

void CPacket::SetRelay()
{
	data[3] = uint8_t(isstream ? '!' : 'Q');
}

void CPacket::ClearRelay()
{
	data[3] = uint8_t(isstream ? ' ' : 'P');
}

bool CPacket::IsRelaySet() const
{
	if (isstream)
		return '!' == char(data[3]);

	return 'Q' == char(data[3]);
}

uint16_t CPacket::GetFrameNumber() const
{
	if (isstream)
		return 0x100u * data[34] + data[35];
	else
		return 0u;
}

void CPacket::SetFrameNumber(uint16_t fn)
{
	if (isstream)
	{
		data[34] = (fn >> 8) & 0xffu;
		data[35] = fn & 0xffu;
	}
}

bool CPacket::IsLastPacket() const
{
	if (isstream and size)
		return 0x8000u == (0x8000u & GetFrameNumber());
	return false;
}

void CPacket::CalcCRC()
{
	if (isstream)
	{
		auto crc = CRC.CalcCRC(data, 52);
		data[52] = uint8_t(crc >> 8);
		data[53] = uint8_t(crc & 0xffu);
	}
	else
	{	// set the CRC for the LSF
		auto crc = CRC.CalcCRC(data+4, 28);
		data[32] = uint8_t(crc >> 8);
		data[33] = uint8_t(crc & 0xffu);
		// now for the payload
		crc = CRC.CalcCRC(data+34, size-36);
		data[size-2] = uint8_t(crc >> 8);
		data[size-1] = uint8_t(crc & 0xffu);
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
