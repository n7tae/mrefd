//
//  cnotification.cpp
//  M17Refd
//
//  Created by Jean-Luc on 05/12/2015.
//  Copyright © 2015 Jean-Luc. All rights reserved.
//
// ----------------------------------------------------------------------------
//    This file is part of M17Refd.
//
//    M17Refd is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    M17Refd is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    with this software.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include "main.h"
#include "notification.h"

////////////////////////////////////////////////////////////////////////////////////////
// constructor

CNotification::CNotification()
{
	// init variables
	m_iId = NOTIFICATION_NONE;
}

CNotification::CNotification(int iId)
{
	m_iId = iId;
}

CNotification::CNotification(int iId, const CCallsign &Callsign)
{
	m_iId = iId;
	m_Callsign = Callsign;
}
