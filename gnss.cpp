/*
 *   Copyright (c) 2025 by Thomas A. Early N7TAE
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>

#include "gnss.h"
#include "jsonkeys.h"

JsonKeys g_keys; // the one and only global def

CGNSS::CGNSS(const uint8_t *data)
{
	Get(data);
}

void CGNSS::Get(const uint8_t *data)
{
	// source type
	source = data[0] >> 4;
	// station type
	station = data[0] & 0xfu;
	// validity
	positionValid = data[1] & 0x80u;
	altitudeValid = data[1] & 0x40u;
	velocityValid = data[1] & 0x20u;
	radiusValid   = data[1] & 0x10u;
	// position
	if (positionValid)
	{
		constexpr double a = 1.0 / (8388607.0);
		auto nlat = (int32_t(data[3] << 24) + (data[4] << 16) + (data[5] << 8)) >> 8;
		latitude = 90.0 * nlat * a;
		auto nlon = (int32_t(data[6] << 24) + (data[7] << 16) + (data[8] << 8)) >> 8;
		longitude = 180.0 * nlon * a;
	}
	// altitude
	if (altitudeValid)
	{
		altitude = 0.5 * (256 * data[9] + data[10]) - 500.0;
	}
	// velocity
	if (velocityValid)
	{
		bearing = 256u * (data[1] & 0x1u) + data[2];
		speed = 0.5 * (16 * data[11] + (data[12] / 16));
	}
	// radius
	if (radiusValid)
	{
		radius = 1;
		for (int i=((data[1]>>1)&7); i>0; radius<<=1, i--)
			;
	}
}

// put a gnss json object on an existing location of a parent object
void CGNSS::MakeJson(nlohmann::json &gnss) const
{
	gnss.clear();
	if (not positionValid)
		return;

	gnss[g_keys.gnss.source] = source;
	gnss[g_keys.gnss.station] = station;

	gnss[g_keys.gnss.position][g_keys.gnss.latitude] = latitude;
	gnss[g_keys.gnss.position][g_keys.gnss.longitude] = longitude;

	if (velocityValid)
	{
		gnss[g_keys.gnss.velocity][g_keys.gnss.bearing] = bearing;
		gnss[g_keys.gnss.velocity][g_keys.gnss.speed] = speed;
	}
	else
		gnss[g_keys.gnss.velocity] = nullptr;

	if (altitudeValid)
		gnss[g_keys.gnss.altitude] = altitude;
	else
		gnss[g_keys.gnss.altitude] = nullptr;

	if (radiusValid)
		gnss[g_keys.gnss.radius] = radius;
	else
		gnss[g_keys.gnss.radius] = nullptr;
}
