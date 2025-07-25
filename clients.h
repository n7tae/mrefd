//
//  Created by Jean-Luc Deltombe (LX3JL) on 31/10/2015.
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

#pragma once

#include <list>
#include <mutex>
#include <memory>

#include "client.h"


////////////////////////////////////////////////////////////////////////////////////////
// define


////////////////////////////////////////////////////////////////////////////////////////
// class

class CClients
{
public:
	// constructors
	CClients();

	// destructors
	virtual ~CClients();

	// locks
	void Lock(void)                     { m_Mutex.lock(); }
	void Unlock(void)                   { m_Mutex.unlock(); }

	// manage Clients
	int     GetSize(void) const         { return (int)m_Clients.size(); }
	void    AddClient(SPClient);
	void    RemoveClient(SPClient);
	bool    IsClient(SPClient) const;

	// pass-through
	std::list<SPClient>::iterator begin()              { return m_Clients.begin(); }
	std::list<SPClient>::iterator end()                { return m_Clients.end(); }
	std::list<SPClient>::const_iterator cbegin() const { return m_Clients.cbegin(); }
	std::list<SPClient>::const_iterator cend()   const { return m_Clients.cend(); }

	// find clients
	SPClient FindClient(const CIp &);

	// iterate on clients
	// all the cleints
	SPClient FindNextClient(std::list<SPClient>::iterator &);
	// all the clients on a module
	SPClient FindNextClient(const char, std::list<SPClient>::iterator &);
	// all the clients with a 8-char callsign and an ip address
	SPClient FindNextClient(const CCallsign &, const CIp &, std::list<SPClient>::iterator &);

protected:
	// data
	std::mutex          m_Mutex;
	std::list<SPClient> m_Clients;
};
