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

#include <string.h>

#include "main.h"
#include "user.h"


////////////////////////////////////////////////////////////////////////////////////////
// constructors

CUser::CUser()
{
	m_LastHeardTime = std::time(nullptr);
}

CUser::CUser(const CCallsign &source, const CCallsign &destination, const CCallsign &reflector)
{
	m_Source = source;
	m_Destination = destination;
	m_Reflector = reflector;
	m_LastHeardTime = std::time(nullptr);
}

CUser::CUser(const CUser &user)
{
	m_Source = user.m_Source;
	m_Destination = user.m_Destination;
	m_Reflector = user.m_Reflector;
	m_LastHeardTime = user.m_LastHeardTime;
}

////////////////////////////////////////////////////////////////////////////////////////
// operators

bool CUser::operator ==(const CUser &user) const
{
	return ((user.m_Source == m_Source) && (user.m_Destination == m_Destination) && (user.m_Reflector == m_Reflector));
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
	xmlFile << "\t<CALLSIGN>" << m_Source << "</CALLSIGN>" << std::endl;
	xmlFile << "\t<VIANODE>" << m_Destination << "</VIANODE>" << std::endl;
	xmlFile << "\t<ONMODULE>" << m_Reflector.GetModule() << "</ONMODULE>" << std::endl;
	xmlFile << "\t<VIAPEER>" << m_Reflector.GetCS(7) << "</VIAPEER>" << std::endl;

	char mbstr[100];
	if (std::strftime(mbstr, sizeof(mbstr), "%A %c", std::localtime(&m_LastHeardTime)))
	{
		xmlFile << "\t<LASTHEARDTIME>" << mbstr << "</LASTHEARDTIME>" << std::endl;
	}
	xmlFile << "</STATION>" << std::endl;
}
