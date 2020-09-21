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

#include "main.h"
#include "reflector.h"

#include <sys/stat.h>


////////////////////////////////////////////////////////////////////////////////////////
// global objects

CReflector  g_Reflector;

////////////////////////////////////////////////////////////////////////////////////////
// function declaration

int main()
{
	const std::string cs(CALLSIGN);
	if ((cs.size() > 7) || (cs.compare(0, 3, "M17")))
	{
		std::cerr << "Malformed reflector callsign: '" << cs << "', aborting!" << std::endl;
		return EXIT_FAILURE;
	}

	// remove pidfile
	remove(PIDFILE_PATH);

	// splash
	std::cout << "Starting " << cs << " " << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_REVISION << std::endl;

	// initialize reflector
	g_Reflector.SetCallsign(cs.c_str());


	// and let it run
	if ( !g_Reflector.Start() )
	{
		std::cout << "Error starting reflector" << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << "Reflector " << g_Reflector.GetCallsign()  << "started and listening" << std::endl;

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
