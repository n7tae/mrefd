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

#include <cstdint>
#include <string>
#include <nlohmann/json.hpp>

class CGNSS
{
public:
	CGNSS() = delete;
	CGNSS(const uint8_t *data);
	virtual ~CGNSS() {}

	void Get(const uint8_t *pdata);
	void MakeJson(nlohmann::json &gpsobj) const;

private:
	// Meta data
	float latitude, longitude, altitude, speed;
	unsigned station, source, bearing, radius;
	bool positionValid, altitudeValid, velocityValid, radiusValid;
};
