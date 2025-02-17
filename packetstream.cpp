//
//  cpacketstream.cpp
//  mrefd
//
//  Created by Jean-Luc Deltombe (LX3JL) on 06/11/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
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

#include "packetstream.h"

CPacketStream::CPacketStream() : CPacketQueue()
{
	m_bOpen = false;
	m_uiStreamId = 0;
	m_OwnerClient = nullptr;
}

bool CPacketStream::OpenPacketStream(const CPacket &Header, std::shared_ptr<CClient>client)
{
	bool ok = false;

	// not already open?
	if ( !m_bOpen )
	{
		// update status
		m_bOpen = true;
		m_uiStreamId = Header.GetStreamId();
		m_Header = Header;
		m_OwnerClient = client;
		m_LastPacketTime.Start();
		ok = true;
	}
	return ok;
}

void CPacketStream::ClosePacketStream(void)
{
	// update status
	m_bOpen = false;
	m_uiStreamId = 0;
	m_OwnerClient = nullptr;
}

const CIp *CPacketStream::GetOwnerIp(void)
{
	if ( m_OwnerClient != nullptr )
	{
		return &(m_OwnerClient->GetIp());
	}
	return nullptr;
}
