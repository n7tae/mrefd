//
//  Created by Jean-Luc Deltombe (LX3JL) on 30/12/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2025 Thomas A. Early, N7TAE
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

#include <mutex>
#include <memory>
#include <unordered_map>

#include "interlink.h"

using InterlinkMap = std::unordered_map<std::string, std::unique_ptr<CInterlink>>;

////////////////////////////////////////////////////////////////////////////////////////
// class

class CInterlinks
{
public:
	// constructor
	CInterlinks();

	// destructor
	~CInterlinks();

	// file io
	virtual bool LoadFromFile(const char *);
	bool ReloadFromFile(void);
	bool NeedReload(void);

	#ifndef NO_DHT
	void Update(const std::string &cs, const std::string &cmods, const std::string &emods, const std::string &ipv4, const std::string &ipv6, uint16_t port, bool islegacy);
	#endif

	// pass-through
	bool empty() const { return m_Imap.empty(); }
	auto begin() { return m_Imap.begin(); }
	auto end()   { return m_Imap.end(); }

	const CInterlink *Find(const std::string &) const;

protected:
#ifndef NO_DHT
	void Emplace(const std::string &cs, const std::string &mods);
#endif
	void Emplace(const std::string &cs, const std::string &mods, const std::string &addr, uint16_t port, bool islegacy);
	bool GetLastModTime(time_t *);
	void ToUpper(std::string &s);

	// data
	mutable std::mutex m_Mutex;
	const char *m_Filename;
	time_t m_LastModTime;
	InterlinkMap m_Imap;
};
