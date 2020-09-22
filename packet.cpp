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

#include "packet.h"

CPacket::CPacket(const uint8_t *buf)
{
	memcpy(m17.magic, buf, sizeof(AM17Frame));
	destination.DecodeCallsign(m17.lich.addr_dst);
	source.DecodeCallsign(m17.lich.addr_src);
}

const CCallsign &CPacket::GetDestCallsign() const
{
	return destination;
}

char CPacket::GetDestModule() const
{
	return destination.GetModule();
}

const CCallsign &CPacket::GetSourceCallsign() const
{
	return source;
}

uint16_t CPacket::GetStreamId() const
{
	return ntohs(m17.streamid);
}

uint16_t CPacket::GetCRC() const
{
	return ntohs(m17.crc);
}

void CPacket::SetCRC(uint16_t crc)
{
	m17.crc = htons(crc);
}

std::unique_ptr<CPacket> CPacket::Duplicate(void) const
{
	return std::unique_ptr<CPacket>(new CPacket(*this));
}

bool CPacket::IsLastPacket() const
{
	return (0x8000u & m17.framenumber == 0x8000u);
}

AM17Frame &CPacket::GetFrame()
{
	return m17;
}
