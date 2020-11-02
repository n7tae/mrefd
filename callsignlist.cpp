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
#include "callsignlist.h"

////////////////////////////////////////////////////////////////////////////////////////
// constructor

CCallsignList::CCallsignList()
{
	m_Filename = nullptr;
	::memset(&m_LastModTime, 0, sizeof(time_t));
}

////////////////////////////////////////////////////////////////////////////////////////
// file io

bool CCallsignList::LoadFromFile(const char *filename)
{
	bool ok = false;
	char sz[256];
	char szStar[2] = "*";

	// and load
	std::ifstream file (filename);
	if ( file.is_open() )
	{
		Lock();

		// empty list
		m_Callsigns.clear();
		// fill with file content
		while ( file.getline(sz, sizeof(sz)).good()  )
		{
			// remove leading & trailing spaces
			char *szt = TrimWhiteSpaces(sz);

			// crack it
			if ( (strlen(szt) > 0) && (szt[0] != '#') )
			{
				// 1st token is callsign
				if ( (szt = strtok(szt, " ,\t")) != nullptr )
				{
					CCallsign callsign(ToUpper(szt));
					// 2nd token is modules list
					szt = strtok(nullptr, " ,\t");
					// if token absent, use wildcard
					if ( szt == nullptr )
					{
						szt = szStar;
					}
					// and add to list
					m_Callsigns.push_back(CCallsignListItem(callsign, CIp(), ToUpper(szt)));
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
		std::cout << "Gatekeeper loaded " << m_Callsigns.size() << " lines from " << filename <<  std::endl;
	}
	else
	{
		std::cout << "Gatekeeper cannot find " << filename <<  std::endl;
	}

	return ok;
}

bool CCallsignList::ReloadFromFile(void)
{
	bool ok = false;

	if ( m_Filename !=  nullptr )
	{
		ok = LoadFromFile(m_Filename);
	}
	return ok;
}

bool CCallsignList::NeedReload(void)
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

bool CCallsignList::IsCallsignListedWithWildcard(const CCallsign &callsign) const
{
	for ( const auto &item : m_Callsigns )
	{
		if (item.HasSameCallsignWithWildcard(callsign))
			return true;
	}

	return false;
}

bool CCallsignList::IsCallsignListedWithWildcard(const CCallsign &callsign, char module) const
{
	for ( const auto &item : m_Callsigns )
	{
		if (item.HasSameCallsignWithWildcard(callsign) && ((module == ' ') || item.HasModuleListed(module)) )
			return true;
	}

	return false;
}

bool CCallsignList::IsCallsignListed(const CCallsign &callsign, char module) const
{
	for ( const auto &item : m_Callsigns )
	{
		if (item.HasSameCallsign(callsign) && item.HasModuleListed(module))
			return true;
	}

	return false;
}

bool CCallsignList::IsCallsignListed(const CCallsign &callsign, const CIp &ip, const char *modules) const
{
	for ( const auto &item : m_Callsigns )
	{
		if (item.HasSameCallsign(callsign) && item.CheckListedModules(modules))
			return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////
// find

CCallsignListItem *CCallsignList::FindListItem(const CCallsign &Callsign)
{
	for ( auto &item : m_Callsigns )
	{
		if ( item.GetCallsign().HasSameCallsign(Callsign) )
		{
			return &item;
		}
	}

	return nullptr;

}

////////////////////////////////////////////////////////////////////////////////////////
// helpers

char *CCallsignList::TrimWhiteSpaces(char *str)
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

bool CCallsignList::GetLastModTime(time_t *time)
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

char *CCallsignList::ToUpper(char *str)
{
	constexpr auto diff = 'a' - 'A';
	for (char *p=str; *p; p++)
	{
		if (*p >= 'a' && *p <= 'z')
			*p -= diff;
	}
	return str;
}
