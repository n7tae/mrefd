//
//  main.cpp
//  m17ref
//
//  Created by Jean-Luc Deltombe (LX3JL) on 31/10/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
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

#include <regex>
#include <sys/stat.h>

#include "main.h"
#include "reflector.h"

////////////////////////////////////////////////////////////////////////////////////////
// global objects

CReflector  g_Reflector;

#ifndef CALLSIGN
#define CALLSIGN "CHANGME"
#endif
#ifndef LISTEN_IPV4
#define LISTEN_IPV4 "none"
#endif
#ifndef LISTEN_IPV6
#define LISTEN_IPV6 "none"
#endif

int main()
{
	auto IPv4RegEx = std::regex("^((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\\.){3,3}(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9]){1,1}$", std::regex::extended);
	auto IPv6RegEx = std::regex("^(([0-9a-fA-F]{1,4}:){7,7}[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,7}:|([0-9a-fA-F]{1,4}:){1,6}(:[0-9a-fA-F]{1,4}){1,1}|([0-9a-fA-F]{1,4}:){1,5}(:[0-9a-fA-F]{1,4}){1,2}|([0-9a-fA-F]{1,4}:){1,4}(:[0-9a-fA-F]{1,4}){1,3}|([0-9a-fA-F]{1,4}:){1,3}(:[0-9a-fA-F]{1,4}){1,4}|([0-9a-fA-F]{1,4}:){1,2}(:[0-9a-fA-F]{1,4}){1,5}|([0-9a-fA-F]{1,4}:){1,1}(:[0-9a-fA-F]{1,4}){1,6}|:((:[0-9a-fA-F]{1,4}){1,7}|:))$", std::regex::extended);
	auto RefRegEx = std::regex("^M17-([A-Z0-9]){3,3}$", std::regex::extended);

	if (! std::regex_match(CALLSIGN, RefRegEx))
	{
		std::cerr << "Malformed reflector callsign: '" << CALLSIGN << "', aborting!" << std::endl;
		return EXIT_FAILURE;
	}

	if (! std::regex_match(LISTEN_IPV4, IPv4RegEx) && ! std::regex_match(LISTEN_IPV6, IPv6RegEx))
	{
		std::cerr << "No valid IP bind address was specifed:" << std::endl;
		std::cerr << "IPv4='" << LISTEN_IPV4 << "' IPV6='" << LISTEN_IPV6 << "'" << std::endl;
		return EXIT_FAILURE;
	}
	const std::string cs(CALLSIGN);

	// remove pidfile
	remove(PIDFILE_PATH);

	// splash
	std::cout << "Starting " << cs << " " << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_REVISION << std::endl;

	// and let it run
	if ( !g_Reflector.Start() )
	{
		std::cout << "Error starting reflector" << std::endl;
		return EXIT_FAILURE;
	}
	std::cout << "Reflector " << g_Reflector.GetCallsign()  << " started and listening" << std::endl;

	// write new pid file
	std::ofstream ofs(PIDFILE_PATH, std::ofstream::out);
	ofs << getpid() << std::endl;
	ofs.close();

	pause(); // wait for any signal

	g_Reflector.Stop();
	std::cout << "Reflector stopped" << std::endl;

	// done
	return EXIT_SUCCESS;
}
