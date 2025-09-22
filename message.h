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

class CMessage
{
public:
	CMessage() { Init(); }
	virtual ~CMessage() {}

	void Init();
	// returns true when the message string is completely constructed
	bool GetBlock(const uint8_t *pdata);
	const std::string &GetMessage();
	void MakeJson(nlohmann::json &gpsobj) const;

private:
	// Meta data
	uint8_t ctl;
	char msg[53];
	std::string str;

	void Clean(char *to, const uint8_t *from);
	void Trim();
};
