//
//  Created by Antony Chazapis (SV9OAN) on 25/2/2018.
//  Copyright © 2016 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020 Thomas A. Early, N7TAE
//
// ----------------------------------------------------------------------------
//    This file is part of mrefd.
//
//    mrefd is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    mrefd is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include "main.h"
#include <string.h>
#include "reflector.h"
#include "m17peer.h"


////////////////////////////////////////////////////////////////////////////////////////
// constructor


CM17Peer::CM17Peer()
{
}

CM17Peer::CM17Peer(const CCallsign &callsign, const CIp &ip, const char *modules)
	: CPeer(callsign, ip, modules)
{
	std::cout << "Adding M17 peer" << std::endl;

	// and construct the M17 clients
	for ( unsigned i = 0; i < ::strlen(modules); i++ )
	{
		// create and append to vector
		m_Clients.push_back(std::make_shared<CM17Client>(callsign, ip, modules[i]));
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// status

bool CM17Peer::IsAlive(void) const
{
	for ( auto it=cbegin(); it!=cend(); it++ )
	{
		if (! (*it)->IsAlive())
			return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////
// revision helper

int CM17Peer::GetProtocolRevision(const CVersion &version)
{
	return version.GetMajor();
}
