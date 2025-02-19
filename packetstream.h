//
//  Created by Jean-Luc Deltombe (LX3JL) on 06/11/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020 Thomas A. Early N7TAE
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
//    with this software.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#pragma once

#include "timer.h"
#include "packet.h"
#include "client.h"

////////////////////////////////////////////////////////////////////////////////////////

//#define STREAM_TIMEOUT      (0.600)
#define STREAM_TIMEOUT      (1.600)

////////////////////////////////////////////////////////////////////////////////////////
// class

class CPacketStream
{
public:
	// constructor
	CPacketStream();

	// open / close
	bool OpenPacketStream(const CPacket &, std::shared_ptr<CClient>);
	void ClosePacketStream(void);

	void Tickle(void)                               { m_LastPacketTime.Start(); }

	// get
	std::shared_ptr<CClient> GetOwnerClient(void)   { return m_OwnerClient; }
	const CIp       &GetOwnerIp(void);
	bool             IsExpired(void) const          { return (m_LastPacketTime.Time() > STREAM_TIMEOUT); }
	bool             IsOpen(void) const             { return m_bOpen; }
	uint16_t         GetPacketStreamId(void) const  { return m_uiStreamId; }

protected:
	// data
	bool                     m_bOpen;
	uint16_t                 m_uiStreamId;
	CTimer                   m_LastPacketTime;
	CPacket                  m_Header;
	std::shared_ptr<CClient> m_OwnerClient;
};
