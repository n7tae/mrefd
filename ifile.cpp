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

#include <fstream>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "configure.h"
#include "ifile.h"

// the global object
CIFileMap g_IFile;
extern CConfigure g_CFG;

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
		while ( file.getline(line, sizeof(line)).good() )
		{
			char *token[4];
			// remove leading & trailing spaces
			token[0] = ToUpper(TrimWhiteSpaces(line));
			// crack it
			if ( (strlen(token[0]) > 0) && (token[0][0] != '#') )
			{
				const char *delim = " \t\r";
				// 1st token is callsign
				if ( (token[0] = strtok(token[0], delim)) != nullptr )
				{
					if (strcmp(token[0], g_CFG.GetCallsign().c_str()))
					{
						if (m_InterlinkMap.end() == m_InterlinkMap.find(token[0]))
						{
							CCallsign callsign(token[0]);
							// read remaining tokens
							for (int i=1; i<4; i++)
							{
								token[i] = strtok(nullptr, delim);
							}

							if (token[2])
							{
								int port = 17000;
								if (token[3])
								{
									port = std::atoi(token[2]);
									if (port < 1024 || port > 49000)
									{
										std::cout << token[0] << " Port " << port << " is out of range, resetting to 17000." << std::endl;
										port = 17000;
									}
									m_InterlinkMap[token[0]] = CIFileItem(callsign, token[1], token[3], (uint16_t)port);
								}
								else
								{
									m_InterlinkMap[token[0]] = CIFileItem(callsign, token[1], token[2], (uint16_t)port);
								}
							}
#ifndef NO_DHT
							else if (token[1])
							{
								m_InterlinkMap[token[0]] = CIFileItem(callsign, token[1]);
							}
#endif
							else
							{
								std::cout << token[0] << " has insufficient parameters!" << std::endl;
							}
						}
						else
						{
							std::cerr << "Duplicate found: " << token[0] << " in " << filename << std::endl;
						}
					}
					else
					{
						std::cerr << "Self linking is not allowed! You cannot use " << token[0] << " in " << filename << std::endl;
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

#ifndef NO_DHT
void CIFileMap::Update(const std::string &cmods, const std::string &cs, const std::string &ipv4, const std::string &ipv6, uint16_t port, const std::string &emods)
{
	auto it = m_InterlinkMap.find(cs);
	if (m_InterlinkMap.end() == it)
	{
		std::cerr << "Can't Update ListenMap item '" << cs << "' because it doesn't exist!";
	}
	else
	{
		it->second.UpdateItem(cmods, ipv4, ipv6, port, emods);
	}
}
#endif
