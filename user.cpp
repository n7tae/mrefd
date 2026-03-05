//
//  Created by Jean-Luc Deltombe (LX3JL) on 13/11/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020 Thomas A. Early
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

#include <iostream>
#include <fstream>
#include <ctime>
#include <string.h>

#include "user.h"

CUser::CUser(const CCallsign &src, const CCallsign &dst, const CCallsign &cli, char module, EMode mode)
: m_Source(src.c_str())
, m_Destination(dst.c_str())
, m_ClientCS(cli.c_str())
, m_OnModule(module)
, m_Mode(mode)
, m_LastHeardTime(std::time(nullptr))
{
}

void CUser::Update(const CCallsign &dst, const CCallsign &cli, char module, EMode mode)
{
	if (m_Destination.compare(dst.c_str()))
		m_Destination.assign(dst.c_str());
	if (m_ClientCS.compare(cli.c_str()))
		m_ClientCS.assign(cli.c_str());
	m_OnModule = module;
	m_Mode = mode;
	m_LastHeardTime = std::time(nullptr);
}

void CUser::Position(const std::string &maid, const std::string &lat, const std::string &lon)
{
	if (m_Maidenhead.compare(maid))
		m_Maidenhead.assign(maid);
	if (m_Latitude.compare(lat))
		m_Latitude.assign(lat);
	if (m_Longitude.compare(lon))
		m_Longitude.assign(lon);
	m_LastHeardTime = std::time(nullptr);
}


////////////////////////////////////////////////////////////////////////////////////////
// reporting

void CUser::AddUser(nlohmann::json &data) const
{
	const std::string mode(((EMode::sm==m_Mode) ? "Stream" : "Packet"));
	const std::string module(1, m_OnModule);
	data += {
		{ "Source",        m_Source        },
		{ "Destination",   m_Destination   },
		{ "Mode",          mode            },
		{ "Via",           m_ClientCS      },
		{ "Module",        module          },
		{ "LastHeardTime", m_LastHeardTime },
		{ "Maidenhead",    m_Maidenhead    },
		{ "Latitude",      m_Latitude      },
		{ "Longitude",     m_Longitude     }
	};
}
