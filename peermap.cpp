//
//  ccallsignlist.cpp
//  m17ref
//
//  Created by Jean-Luc Deltombe (LX3JL) on 30/12/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020 Thomas A. Early, N7TAE
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

#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "main.h"
#include "peermap.h"

////////////////////////////////////////////////////////////////////////////////////////
// constructors

CPeerMap::CPeerMap()
{
	m_Filename = nullptr;
	::memset(&m_LastModTime, 0, sizeof(time_t));
}

////////////////////////////////////////////////////////////////////////////////////////
// file io

bool CPeerMap::LoadFromFile(const char *filename)
{
	bool ok = false;
	char line[256];

	// and load
	std::ifstream file (filename);
	if ( file.is_open() )
	{
		Lock();

		// empty list
		m_Peers.clear();
		// fill with file content
		while ( file.getline(line, sizeof(line)).good()  )
		{
			// remove leading & trailing spaces
			auto name = TrimWhiteSpaces(line);
			// crack it
			if ( (strlen(name) > 0) && (name[0] != '#') )
			{
				// 1st token is callsign
				if ( (name = strtok(name, " ,\t")) != nullptr )
				{
					if (strcmp(name, CALLSIGN))
					{
						if (m_Peers.end() == m_Peers.find(name))
						{
							CCallsign callsign(ToUpper(name));
							// 2nd token is ip
							char *szip, *szmods;
							if ( (szip = strtok(nullptr, " ,\t")) != nullptr )
							{
								// 3rd token is modules list
								if ( (szmods = strtok(nullptr, " ,\t")) != nullptr )
								{
									// create and and store
									m_Peers[name] = CPeerMapItem(callsign, szip, ToUpper(szmods));
								}
								else
								{
									std::cerr << "No modules defined for peeer " << name << std::endl;
								}
							}
							else
							{
								std::cerr << "No IP address defined for peer " << name << std::endl;
							}
						}
						else
						{
							std::cerr << "Duplicate found: " << name << " in " << filename << std::endl;
						}
					}
					else
					{
						std::cerr << "Self linking is not allowed! You cannot use " << name << " in " << filename << std::endl;
					}
				}
			}
		}
		// close file
		file.close();

		// keep file path
		m_Filename = filename;

		// update time
		GetLastModTime(&m_LastModTime);

		// and done
		Unlock();
		ok = true;
		std::cout << "Gatekeeper loaded " << m_Peers.size() << " lines from " << filename <<  std::endl;
	}
	else
	{
		std::cout << "Gatekeeper cannot find " << filename <<  std::endl;
	}

	return ok;
}

bool CPeerMap::ReloadFromFile(void)
{
	bool ok = false;

	if ( m_Filename !=  nullptr )
	{
		ok = LoadFromFile(m_Filename);
	}
	return ok;
}

bool CPeerMap::NeedReload(void)
{
	bool needReload = false;

	time_t time;
	if ( GetLastModTime(&time) )
	{
		needReload = time != m_LastModTime;
	}
	return needReload;
}

////////////////////////////////////////////////////////////////////////////////////////
// compare

bool CPeerMap::IsCallsignListed(const CCallsign &callsign, char module) const
{
	for ( const auto &item : m_Peers )
	{
		if (item.second.HasSameCallsign(callsign) && item.second.HasModuleListed(module))
			return true;
	}

	return false;
}

bool CPeerMap::IsCallsignListed(const CCallsign &callsign, const CIp &ip, const char *modules) const
{
	for ( const auto &item : m_Peers )
	{
		if ( item.second.HasSameCallsign(callsign) )
		{
			if ( item.second.CheckListedModules(modules) )
			{
				if ( ip == item.second.GetIp() )
				{
					return true;
				}
			}
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////
// find

CPeerMapItem *CPeerMap::FindMapItem(const std::string &cs)
{
	auto it = m_Peers.find(cs);
	if (m_Peers.end() == it)
		return nullptr;
	return &it->second;
}

////////////////////////////////////////////////////////////////////////////////////////
// helpers

char *CPeerMap::TrimWhiteSpaces(char *str)
{
	char *end;

	// Trim leading space & tabs
	while((*str == ' ') || (*str == '\t')) str++;

	// All spaces?
	if(*str == 0)
		return str;

	// Trim trailing space, tab or lf
	end = str + ::strlen(str) - 1;
	while((end > str) && ((*end == ' ') || (*end == '\t') || (*end == '\r'))) end--;

	// Write new null terminator
	*(end+1) = 0;

	return str;
}

bool CPeerMap::GetLastModTime(time_t *time)
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

char *CPeerMap::ToUpper(char *str)
{
	constexpr auto diff = 'a' - 'A';
	for (char *p=str; *p; p++)
	{
		if (*p >= 'a' && *p <= 'z')
			*p -= diff;
	}
	return str;
}
