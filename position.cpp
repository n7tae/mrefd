/*
				  mrefd - An M17-only refector
				Copyright (C) 2026 Thomas A. Early

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <cstdio>

#include "position.h"

CPosition::CPosition(const uint8_t *data)
{
	Get(data);
}

void CPosition::Get(const uint8_t *data)
{
	// source type
	source = data[0] >> 4;
	// station type
	station = data[0] & 0xfu;
	// validity
	positionValid = data[1] & 0x80u;
	// position
	if (positionValid)
	{
		constexpr double a = 1.0 / 8388607.0;
		auto nlat = (int32_t(data[3] << 24) + (data[4] << 16) + (data[5] << 8)) >> 8;
		latitude = 90.0 * nlat * a;
		auto nlon = (int32_t(data[6] << 24) + (data[7] << 16) + (data[8] << 8)) >> 8;
		longitude = 180.0 * nlon * a;
	}
}

const char *CPosition::GetSource() const
{
	switch (source)
	{
		case 0:  return "M17 client";
		case 1:  return "OpenRTx";
		case 15: return "Other";
		default: return "Reserved";
	}
}

const char *CPosition::GetStation() const
{
	switch (station)
	{
		case 0:  return "Fixed";
		case 1:  return "Mobile";
		case 2:  return "Handheld";
		case 15: return "Other";
		default: return "Reserved";
	}
}

const char *CPosition::GetPosition(std::string &la, std::string &lo)
{
	if (positionValid)
	{
		double lat = latitude + 90.0;
		double lon = longitude + 180.0;
		maidenhead[0] = 'A' +  (int(lon) / 20);
		maidenhead[1] = 'A' +  (int(lat) / 10);
		maidenhead[2] = '0' + ((int(lon) % 20) / 2);
		maidenhead[3] = '0' +  (int(lat) % 10);
		maidenhead[4] = 'a' +  (int(lon * 12.0) % 24);
		maidenhead[5] = 'a' +  (int(lat * 24.0) % 24);
		maidenhead[6] = '\0';
		char buf[16];
		snprintf(buf, 15, "%+.6f", latitude);
		la.assign(buf);
		snprintf(buf, 15, "%+.6f", longitude);
		lo.assign(buf);
		return maidenhead;
	}
	return nullptr;
}
