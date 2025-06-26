//
//  ccallsignlist.cpp
//  mrefd
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

#include <fstream>
#include <sstream>
#include <string>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <vector>
#include <regex>

#include "configure.h"
#include "interlinks.h"

// the global object
CInterlinks g_Interlinks;
extern CConfigure g_CFG;

CInterlinks::CInterlinks()
{
	m_Filename = nullptr;
	::memset(&m_LastModTime, 0, sizeof(time_t));
}

CInterlinks::~CInterlinks()
{
	for (auto &item : m_Imap)
	{
		item.second.reset();
	}
	m_Imap.clear();
}

bool CInterlinks::LoadFromFile(const char *filename)
{
	bool ok = false;
	std::string line;

	// and load
	std::ifstream file(filename);
	if ( file.is_open() )
	{
		// empty list
		m_Imap.clear();
		// fill with file content
		unsigned count = 0;
		while (std::getline(file, line))
		{
			count++;
			std::stringstream ss(line);
			std::vector<std::string> v;
			std::string item;
			while (ss >> item)
				v.push_back(item);
			if (0 == v.size() or '#' == v[0][0])
				continue;
			// make sure the callsign and modules are uppercase
			ToUpper(v[0]);        // the first word
			ToUpper(*v.rbegin()); // the last word
			// check for self-linking
			if (std::string::npos != v[0].find(g_CFG.GetCallsign()))
			{
				std::cerr << m_Filename << " line #" << count << ": Self linking is not allowed! You cannot use " << v[0] << std::endl;
				continue;
			}
			if (not std::regex_match(v[0], std::regex("^[L]?M17-([A-Z0-9]){3,3}$", std::regex::extended)))
			{
				std::cerr << m_Filename << " line #" << count << ": malformed reflect :" << v[0] << std::endl;
				continue;
			}

			bool islegacy = false;

			if (v[0].size() > 7)
			{
				islegacy = true;
				v[0].assign(v[0].substr(1));
			}

			switch(v.size())
			{
			default:
				std::cerr << m_Filename << " line #" << count << ": Bad input line in " << filename << ": " << line << std::endl;
				break;
			case 2:	// only for DHT-enabled systems
#ifdef NO_DHT
				std::cout << "ERROR: You haven't enabled DHT support, so you can't connect to " << v[0] << " with just two paramters" << std::endl;
#else
				Emplace(v[0], v[1]);
#endif
				break;
			case 3:	// supply the default connection port
				Emplace(v[0], v[2], v[1], 17000u, islegacy);
				break;
			case 4:
				uint16_t port = std::stoul(v[2]);
				if (port < 1024u or port > 49000u)
				{
					std::cout << m_Filename << " line #" << count << ": Resetting port for " << v[0] << " from " << v[2] << " to 17000" << std::endl;
					port = 17000u;
				}
				Emplace(v[0], v[3], v[1], port, islegacy);
				break;
			}
		}
		// close file
		file.close();

		// keep file path
		m_Filename = filename;

		// update time
		GetLastModTime(&m_LastModTime);

		// and done
		ok = true;
		std::cout << "Gatekeeper loaded " << m_Imap.size() << " lines from " << filename <<  std::endl;
	}
	else
	{
		std::cout << "Gatekeeper cannot find " << filename <<  std::endl;
	}

	return ok;
}

bool CInterlinks::ReloadFromFile(void)
{
	bool ok = false;

	if ( m_Filename !=  nullptr )
	{
		ok = LoadFromFile(m_Filename);
	}
	return ok;
}

bool CInterlinks::NeedReload(void)
{
	bool needReload = false;

	time_t time;
	if ( GetLastModTime(&time) )
	{
		needReload = time != m_LastModTime;
	}
	return needReload;
}

bool CInterlinks::IsCallsignListed(const std::string &cs, const std::string &addr) const
{
	const auto &item = m_Imap.find(cs);
	if (m_Imap.end() != item)
	{
		return (addr == item->second->GetIPv4() or addr == item->second->GetIPv6());
	}

	return false;
}

const CInterlink *CInterlinks::Find(const std::string &cs) const
{
	auto item = m_Imap.find(cs);
	if (m_Imap.end() == item)
		return nullptr;
	else
		return item->second.get();
}

bool CInterlinks::GetLastModTime(time_t *time)
{
	bool ok = false;

	if ( m_Filename != nullptr )
	{
		struct stat fileStat;
		if( ::stat(m_Filename, &fileStat) != -1 )
		{
			*time = fileStat.st_mtime;
			ok = true;
		}
	}
	return ok;
}

void CInterlinks::ToUpper(std::string &str)
{
	for (auto p=str.begin(); p!=str.end(); p++)
	{
		if (islower(*p))
			*p = toupper(*p);
	}
}

#ifndef NO_DHT
void CInterlinks::Update(const std::string &cs, const std::string &cmods, const std::string &emods, const std::string &ipv4, const std::string &ipv6, uint16_t port, bool islegacy)
{
	auto item = m_Imap.find(cs);
	if (m_Imap.end() != item)
	{
		item->second->UpdateItem(cmods, emods, ipv4, ipv6, port, islegacy);
		return;
	}
	std::cerr << "ERROR: Can't Update CInterlinks item '" << cs << "' because it doesn't exist!";
}
#endif

void CInterlinks::Emplace(const std::string &cs, const std::string &mods)
{
	auto item = m_Imap.emplace(cs, std::make_unique<CInterlink>(cs, mods));
	if (not item.second)
		std::cout << cs << " was already defined earlier. This will be ignored." << std::endl;
}

void CInterlinks::Emplace(const std::string &cs, const std::string &mods, const std::string &addr, uint16_t port, bool islegacy)
{
	auto item = m_Imap.emplace(cs, std::make_unique<CInterlink>(cs, mods, addr, port, islegacy));
	if (not item.second)
		std::cout << cs << " was already defined earlier. This will be ignored." << std::endl;
}
