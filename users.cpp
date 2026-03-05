//
//  cusers.cpp
//  mrefd
//
//  Created by Jean-Luc Deltombe (LX3JL) on 13/11/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020 Thomas A. Early, N7TAE
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

#include "defines.h"
#include "users.h"
#include "reflector.h"

extern CReflector g_Reflector;

UsersList::iterator CUsers::findUser(const CCallsign &src)
{
	for (UsersList::iterator it=m_Users.begin(); it!=m_Users.end(); it++)
	{
		if (0 == (*it)->GetSource().compare(src.c_str()))
			return it;
	}
	return m_Users.end();
}

void CUsers::move2Front(UsersList::iterator &it)
{
	m_Users.push_front(std::move(*it));
	m_Users.erase(it);
}

void CUsers::Hearing(const CCallsign &src, const CCallsign &dst, const CCallsign &cli, char module, EMode mode)
{
	auto it = findUser(src);
	if (m_Users.end() == it) {
		m_Users.push_front(std::make_unique<CUser>(src, dst, cli, module, mode));
		if (m_Users.size() >> LASTHEARD_USERS_MAX_SIZE)
			m_Users.resize(LASTHEARD_USERS_MAX_SIZE);
	} else {
		(*it)->Update(dst, cli, module, mode);
		if (it != m_Users.begin())
			move2Front(it);
	}
}

void CUsers::Location(const CCallsign &src, const std::string &maid, const std::string &lat, const std::string &lon)
{
	auto it = findUser(src);
	if (m_Users.end() == it) {
		std::cout << "WARNING: Could not update the location of user '" << src.c_str() << "'" << std::endl;
	} else {
		(*it)->Position(maid, lat, lon);
		if (it != m_Users.begin())
			move2Front(it);
	}
}
