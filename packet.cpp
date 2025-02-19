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

#include <arpa/inet.h>

#include "packet.h"
#include "crc.h"

static const CCRC CRC;

CPacket::CPacket()
{
	memset(data, MAX_PACKET_SIZE + 1, 0);
	size = 0;
}

void CPacket::SetSize(size_t n, bool bis)
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

// returns the StreamID in host byte order
uint16_t CPacket::GetStreamId() const
{
	return isstream ? 0x100u * data[4] + data[5] : 0u;
}

// returns LSD:TYPE in host byte order
uint16_t CPacket::GetFrameType() const
{
	if (isstream)
		return 0x100u * data[18] + data[19];

	return 0x100u * data[16] + data[17];
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

uint16_t CPacket::GetFrameNumber()
{
	if (isstream)
		return 0x100u * data[34] + data[35];
	else
		return 0u;
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
		auto crc = CRC.CalcCRC(data, 28);
		data[32] = uint8_t(crc >> 8);
		data[33] = uint8_t(crc & 0xffu);
	}
}
