//
//  cuser.h
//  m17ref
//
//  Created by Jean-Luc Deltombe (LX3JL) on 13/11/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//
// ----------------------------------------------------------------------------
//    This file is part of m17ref.
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

#pragma once

#include <ctime>
#include <memory>

#include "callsign.h"
#include "client.h"

enum class EMode { pm, sm }; // stream or packet mode

class CUser
{
public:
	// constructor
	CUser();
	CUser(const std::string, const std::string, const std::string, char, EMode);

	// get data
	const std::string &GetSource(void)        const { return m_Source; }
	const std::string &GetDestination(void)   const { return m_Destination; }
	const std::string &GetClient(void)        const { return m_ClientCS; }
	std::time_t GetLastHeardTime(void) const { return m_LastHeardTime; }
	bool IsStreamMode(void)            const { return m_Mode == EMode::sm; }
	std::string GetClientCS(void)      const { return m_ClientCS; }
	char GetModule(void)               const { return m_OnModule; }

	// operation
	void HeardNow(void)     { m_LastHeardTime = std::time(nullptr); }

	// operators
	bool operator ==(const CUser &) const;
	bool operator <(const CUser &) const;

	// reporting
	void WriteXml(std::ofstream &);

protected:
	// data
	const std::string m_Source, m_Destination, m_ClientCS;
	char m_OnModule;
	EMode m_Mode;
	std::time_t m_LastHeardTime;
};
