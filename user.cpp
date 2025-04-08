//
//  Created by Jean-Luc Deltombe (LX3JL) on 13/11/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020 Thomas A. Early
//
// ----------------------------------------------------------------------------
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

#include <iostream>
#include <fstream>
#include <ctime>
#include <string.h>

#include "user.h"


////////////////////////////////////////////////////////////////////////////////////////
// constructors

CUser::CUser()
{
	m_LastHeardTime = std::time(nullptr);
}

CUser::CUser(const std::string src, const std::string dst, const std::string cli, char mod, EMode mode)
: m_Source(src)
, m_Destination(dst)
, m_ClientCS(cli)
, m_OnModule(mod)
, m_Mode(mode)
{
	m_LastHeardTime = std::time(nullptr);
}

////////////////////////////////////////////////////////////////////////////////////////
// operators

bool CUser::operator ==(const CUser &user) const
{
	return ((user.m_Source == m_Source) and (user.m_Destination == m_Destination) and (user.m_ClientCS == m_ClientCS));
}


bool CUser::operator <(const CUser &user) const
{
	// smallest is youngest
	return (std::difftime(m_LastHeardTime, user.m_LastHeardTime) > 0);
}

////////////////////////////////////////////////////////////////////////////////////////
// reporting

void CUser::WriteXml(std::ofstream &xmlFile)
{
	xmlFile << "<STATION>" << std::endl;
	xmlFile << "\t<SOURCE>" << m_Source << "</SOURCE>" << std::endl;
	xmlFile << "\t<DESTINATION>" << m_Destination << "</DESTINATION>" << std::endl;
	xmlFile << "\t<MODE>";
	if (m_Mode == EMode::sm)
		xmlFile << "Stream";
	else
		xmlFile << "Packet";
	xmlFile << "</MODE>" << std::endl;
	xmlFile << "\t<VIA>" << m_ClientCS << "</VIA>" << std::endl;
	xmlFile << "\t<ONMODULE>" << m_OnModule << "</ONMODULE>" << std::endl;

	char mbstr[100];
	if (std::strftime(mbstr, sizeof(mbstr), "%FT%TZ", std::gmtime(&m_LastHeardTime)))
	{
		xmlFile << "\t<LASTHEARDTIME>" << mbstr << "</LASTHEARDTIME>" << std::endl;
	}
	xmlFile << "</STATION>" << std::endl;
}
