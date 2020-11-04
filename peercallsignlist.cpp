//
//  cxlxcallsignlist.cpp
//  m17ref
//
//  Created by Jean-Luc Deltombe (LX3JL) on 31/01/2016.
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

#include <string.h>
#include "main.h"
#include "peercallsignlist.h"


////////////////////////////////////////////////////////////////////////////////////////
// file io

bool CPeerCallsignList::LoadFromFile(const char *filename)
{
	bool ok = false;
	char sz[256];

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
					// 2nd token is ip
					char *szip;
					if ( (szip = strtok(nullptr, " ,\t")) != nullptr )
					{
						// 3rd token is modules list
						if ( (szt = strtok(nullptr, " ,\t")) != nullptr )
						{
							// create and and store
							m_Callsigns.push_back(CCallsignListItem(callsign, szip, ToUpper(szt)));
						}
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
		std::cout << "Gatekeeper loaded " << m_Callsigns.size() << " lines from " << filename <<  std::endl;

		Lock();
		std::cout << "PEER ENUMERATION: Count=" << m_Callsigns.size() << std::endl;
		for (auto it = m_Callsigns.begin(); it != m_Callsigns.end(); it++)
			std::cout << (*it).GetCallsign() << std::endl;
		Unlock();
	}
	else
	{
		std::cout << "Gatekeeper cannot find " << filename <<  std::endl;
	}

	return ok;
}
