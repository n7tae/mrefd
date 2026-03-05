//
//  Created by Jean-Luc Deltombe (LX3JL) on 13/11/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020 Thomas A. Early, N7TAE
//
// ----------------------------------------------------------------------------
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

#include <list>
#include <mutex>
#include <memory>

#include "user.h"

using UsersList = std::list<std::unique_ptr<CUser>>;

////////////////////////////////////////////////////////////////////////////////////////

class CUsers
{
public:
	// locks
	void Lock(void)                     { m_Mutex.lock(); }
	void Unlock(void)                   { m_Mutex.unlock(); }

	// management
	int    GetSize(void) const          { return (int)m_Users.size(); }

	// pass-through
	UsersList::const_iterator cbegin()  { return m_Users.cbegin(); }
	UsersList::const_iterator cend()    { return m_Users.cend(); }

	// operation
	void Hearing(const CCallsign &src, const CCallsign &dst, const CCallsign &cli, char module, EMode mode);
	void Location(const CCallsign &src, const std::string &maid, const std::string &lat, const std::string &lon);

protected:
	void move2Front(UsersList::iterator &it);
	UsersList::iterator findUser(const CCallsign &src);
	// data
	std::mutex m_Mutex;
	UsersList  m_Users;
};
