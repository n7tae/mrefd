//
//  ccallsignlist.cpp
//  m17ref
//
//  Created by Jean-Luc Deltombe (LX3JL) on 30/12/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020,2022 Thomas A. Early, N7TAE
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
#include "configure.h"
#include "ifile.h"

// the global object
CIFileMap g_IFile;

CIFileMap::CIFileMap()
{
	m_Filename = nullptr;
	::memset(&m_LastModTime, 0, sizeof(time_t));
}

bool CIFileMap::LoadFromFile(const char *filename)
{
	bool ok = false;
	char line[256];

	// and load
	std::ifstream file (filename);
	if ( file.is_open() )
	{
		Lock();

		// empty list
		m_InterlinkMap.clear();
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
					if (strcmp(name, g_CFG.GetCallsign().c_str()))
					{
						if (m_InterlinkMap.end() == m_InterlinkMap.find(name))
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
									m_InterlinkMap[name] = CIFileItem(callsign, szip, ToUpper(szmods));
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
		Unlock();
		// close file
		file.close();

		// keep file path
		m_Filename = filename;

		// update time
		GetLastModTime(&m_LastModTime);

		// and done
		ok = true;
		std::cout << "Gatekeeper loaded " << m_InterlinkMap.size() << " lines from " << filename <<  std::endl;
	}
	else
	{
		std::cout << "Gatekeeper cannot find " << filename <<  std::endl;
	}

	return ok;
}

bool CIFileMap::ReloadFromFile(void)
{
	bool ok = false;

	if ( m_Filename !=  nullptr )
	{
		ok = LoadFromFile(m_Filename);
	}
	return ok;
}

bool CIFileMap::NeedReload(void)
{
	bool needReload = false;

	time_t time;
	if ( GetLastModTime(&time) )
	{
		needReload = time != m_LastModTime;
	}
	return needReload;
}

bool CIFileMap::IsCallsignListed(const CCallsign &callsign, char module) const
{
	for ( const auto &item : m_InterlinkMap )
	{
		if (item.second.HasSameCallsign(callsign) && item.second.HasModuleListed(module))
			return true;
	}

	return false;
}

bool CIFileMap::IsCallsignListed(const CCallsign &callsign, const CIp &ip, const char *modules) const
{
	for ( const auto &item : m_InterlinkMap )
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

CIFileItem *CIFileMap::FindMapItem(const std::string &cs)
{
	auto it = m_InterlinkMap.find(cs);
	if (m_InterlinkMap.end() == it)
		return nullptr;
	return &it->second;
}

char *CIFileMap::TrimWhiteSpaces(char *str)
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

bool CIFileMap::GetLastModTime(time_t *time)
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

char *CIFileMap::ToUpper(char *str)
{
	constexpr auto diff = 'a' - 'A';
	for (char *p=str; *p; p++)
	{
		if (*p >= 'a' && *p <= 'z')
			*p -= diff;
	}
	return str;
}
