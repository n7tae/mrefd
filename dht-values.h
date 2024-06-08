/*
 *   Copyright (c) 2022-2024 by Thomas A. Early N7TAE
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#pragma once

#include <opendht.h>

// comment out sections you don't need for your application
#define USE_MREFD_VALUES
//#define USE_URFD_VALUES

// a typesafe way to extract the numeric value from a enum class
// note that this is a constexpr and so can even be used in an
// array declaration or as a tuple index
template<typename E> constexpr auto toUType(E enumerator) noexcept
{
	return static_cast<std::underlying_type_t<E>>(enumerator);
} // Item #10 in "Effective Modern C++", by Scott Meyers, O'REILLY

#ifdef USE_MREFD_VALUES

// DHT::value::user_type for mrefd values
// These are the value release version strings in a
// preprocessor definition for your convience
// if your app needs a particular part, then it should handle all versions of that part
#define MREFD_CONFIG_1  "mrefd-config-1"

// dht::Value ids of the different parts of the mrefd document
// can be assigned any unsigned value except 0
// more parts can be added, but don't change the value of any existing part
// using toUType, you can set or query a user_type to determine the value part
// this can be done before unpacking the MSGPACK
enum class EMrefdValueID : uint64_t { Config=1, Peers=2, Clients=3, Users=4 };

///////////////// MREFD PART VALUES ///////////////

// the configuration part
struct SMrefdConfig1 // user_type is MREFD_CONFIG_1
{
	std::time_t timestamp;     // when this value was set
	std::string callsign;      // the callsign of the mrefd reflector
	std::string ipv4addr;      // the external IPv4 address
	std::string ipv6addr;      // the external IPv6 address
	std::string modules;       // all the configured modules, [A-Z]
	std::string encryptedmods; // modules that will pass encrypted streams
	std::string url;           // the URL of the dashboard
	std::string email;         // the email of the responsible owner
	std::string sponsor;       // the organization or individual sponsoring the reflector
	std::string country;       // the 2-letter country code
	std::string version;       // the version of the reflector software
	uint16_t port;             // the connection listening UDP port, usually 17000

	// the order in which MSGPACK serializes the data
	MSGPACK_DEFINE(timestamp, callsign, ipv4addr, ipv6addr, modules, encryptedmods, url, email, sponsor, country, version, port)
};

#endif	// USE_MREFD_VALUES

#ifdef USE_URFD_VALUES

// DHT::value::user_type for urfd values
// These are the value release version strings in a
// preprocessor definition for your convience
// if your app needs a particular part, then it should handle all versions of that part
#define URFD_CONFIG_1  "urfd-config-1"
#define URFD_CONFIG_2  "urfd-config-2"

// dht::Value::id of the different parts of the urfd document
// can be assigned any unsigned value except 0
// more parts can be added, but don't change the value of any existing part
// using toUType, you can set or query a user_type to determine the value part
// this can be done before unpacking the MSGPACK
enum class EUrfdValueID : uint64_t { Config=1, Peers=2, Clients=3, Users=4 };

// the following enum classes can be used to reference a particular value in a fixed array
// 'SIZE' has to be last value for these scoped enums as this is used to declare these arrays
//
// all the configurable ports in urfd (G3 and BM are not configurable)
enum class EUrfdPorts  : unsigned { dcs, dextra, dmrplus, dplus, m17, mmdvm, nxdn, p25, urf, ysf, SIZE };
// autolink modules for these protocols
enum class EUrfdAlMod  : unsigned { nxdn, p25, ysf, SIZE };
// default TX/RX values for ysf
enum class EUrfdTxRx   : unsigned { rx, tx, SIZE };
// reflector ID values for these two modes
enum class EUrfdRefId  : unsigned { nxdn, p25, SIZE };

struct SUrfdConfig1 // user_type is URFD_CONFIG_1
{
	std::time_t timestamp;
	std::string callsign, ipv4addr, ipv6addr, modules, transcodedmods, url, email, sponsor, country, version;
	// transcodedmods are those modules that support full transcoding
	std::array<uint16_t, toUType(EUrfdPorts::SIZE)> port;
	std::array<char, toUType(EUrfdAlMod::SIZE)> almod;
	std::array<unsigned long, toUType(EUrfdTxRx::SIZE)> ysffreq;
	std::array<unsigned, toUType(EUrfdRefId::SIZE)> refid;
	std::unordered_map<char, std::string> description;
	bool g3enabled;

	MSGPACK_DEFINE(timestamp, callsign, ipv4addr, ipv6addr, modules, transcodedmods, url, email, sponsor, country, version, almod, ysffreq, refid, g3enabled, port, description)
};

enum class EUrfdPorts2 : unsigned { dcs, dextra, dmrplus, dplus, dsd, m17, mmdvm, nxdn, p25, urf, ysf, SIZE };

struct SUrfdConfig2
{
	std::time_t timestamp;
	std::string callsign, ipv4addr, ipv6addr, modules, transcodedmods, url, email, sponsor, country, version;
	std::array<uint16_t, toUType(EUrfdPorts2::SIZE)> port;
	std::array<char, toUType(EUrfdAlMod::SIZE)> almod;
	std::array<unsigned long, toUType(EUrfdTxRx::SIZE)> ysffreq;
	std::array<unsigned, toUType(EUrfdRefId::SIZE)> refid;
	std::unordered_map<char, std::string> description;
	bool g3enabled;

	MSGPACK_DEFINE(timestamp, callsign, ipv4addr, ipv6addr, modules, transcodedmods, url, email, sponsor, country, version, almod, ysffreq, refid, g3enabled, port, description)
};

#endif // USE_URFD_VALUES
