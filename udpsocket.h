//
//  Created by Jean-Luc Deltombe (LX3JL) on 31/10/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020,2025 Thomas A. Early, N7TAE
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

#include <cstdint>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include "ip.h"

////////////////////////////////////////////////////////////////////////////////////////
// define

#define UDP_BUFFER_LENMAX       1024


////////////////////////////////////////////////////////////////////////////////////////
// class

class CUdpSocket
{
public:
	// constructor
	CUdpSocket();

	// destructor
	~CUdpSocket();

	// open & close
	bool Open(const CIp &Ip);
	void Close(void);
	int  GetSocket(void)
	{
		return m_fd;
	}

	// read
	unsigned Receive    (uint8_t *, CIp &, int);
	unsigned ReceiveFrom(uint8_t *, CIp &);

	// write
	unsigned Send(const uint8_t *, size_t, const CIp &) const;

private:
	// data
	int m_fd;
	CIp m_addr;
};
