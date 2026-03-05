//
//  cuser.h
//  mrefd
//
//  Created by Jean-Luc Deltombe (LX3JL) on 13/11/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//
// ----------------------------------------------------------------------------
//    This file is part of mrefd.
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

#include <ctime>
#include <memory>
#include <nlohmann/json.hpp>

#include "callsign.h"
#include "client.h"

enum class EMode { pm, sm }; // stream or packet mode

class CUser
{
public:
	// constructor
	CUser() = delete;
	CUser(const CCallsign &src, const CCallsign &dst, const CCallsign &cli, char module, EMode mode);
	const std::string &GetSource() { return m_Source; }
	void Update(const CCallsign &dst, const CCallsign &cli, char module, EMode mode);
	void Position(const std::string &maid, const std::string &lat, const std::string &lon);

	// reporting
	void AddUser(nlohmann::json &data) const;

protected:
	// data
	std::string m_Source, m_Destination, m_ClientCS, m_Maidenhead, m_Latitude, m_Longitude;
	char m_OnModule;
	EMode m_Mode;
	std::time_t m_LastHeardTime;
};
