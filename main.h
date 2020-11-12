//
//  Created by Jean-Luc Deltombe (LX3JL) on 31/10/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020 Thomas A. Eary, N7TAE
//
// ----------------------------------------------------------------------------
//    This file is part of m17ref.
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

#pragma once

#include <atomic>
#include <array>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <chrono>
#include <future>
#include <mutex>
#include <memory>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <ctime>
#include <cctype>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <arpa/inet.h>

#include "configure.h"

////////////////////////////////////////////////////////////////////////////////////////
// defines

// version -----------------------------------------------------

#define VERSION_MAJOR                   0
#define VERSION_MINOR                   3
#define VERSION_REVISION                4

// global ------------------------------------------------------

//#define JSON_MONITOR

// debug -------------------------------------------------------

//#define DEBUG_NO_ERROR_ON_XML_OPEN_FAIL
//#define DEBUG_DUMPFILE

// protocols ---------------------------------------------------

// M17
#define M17_PORT                        17000
#define M17_KEEPALIVE_PERIOD			3
#define M17_KEEPALIVE_TIMEOUT           (M17_KEEPALIVE_PERIOD*10)
#define M17_RECONNECT_PERIOD            5

// xml & json reporting -----------------------------------------

#define LASTHEARD_USERS_MAX_SIZE        100
#define XML_UPDATE_PERIOD               10                              // in seconds

// system paths -------------------------------------------------
#define XML_PATH                        "/var/log/mrefd.xml"
#define WHITELIST_PATH                  "/usr/local/etc/mrefd.whitelist"
#define BLACKLIST_PATH                  "/usr/local/etc/mrefd.blacklist"
#define INTERLINKLIST_PATH              "/usr/local/etc/mrefd.interlink"
#define PIDFILE_PATH                    "/var/run/mrefd.pid"

////////////////////////////////////////////////////////////////////////////////////////
// typedefs


////////////////////////////////////////////////////////////////////////////////////////
// macros

#define MIN(a,b) 				(int(a) < int(b))?(a):(b)
#define MAX(a,b) 				(int(a) > int(b))?(a):(b)
#define MAKEWORD(low, high)		((uint16_t)(((uint8_t)(low)) | (((uint16_t)((uint8_t)(high))) << 8)))
#define MAKEDWORD(low, high)	((uint32_t)(((uint16_t)(low)) | (((uint32_t)((uint16_t)(high))) << 16)))
#define LOBYTE(w)				((uint8_t)(uint16_t)(w & 0x00FF))
#define HIBYTE(w)				((uint8_t)((((uint16_t)(w)) >> 8) & 0xFF))
#define LOWORD(dw)				((uint16_t)(uint32_t)(dw & 0x0000FFFF))
#define HIWORD(dw)				((uint16_t)((((uint32_t)(dw)) >> 16) & 0xFFFF))

////////////////////////////////////////////////////////////////////////////////////////
// global objects

class CReflector;
extern CReflector  g_Reflector;

class CGateKeeper;
extern CGateKeeper g_GateKeeper;
