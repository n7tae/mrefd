//
//  Copyright © 2022 Thomas A. Early, N7TAE
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

#include <iostream>
#include <fstream>
#include <regex>
#include <sys/stat.h>
#include <csignal>

#include "reflector.h"
#include "configure.h"
#include "version.h"

extern CConfigure g_CFG;
extern CVersion g_Version;
extern CReflector g_Reflector;

void SigHandler(int sig)
{
	switch (sig)
	{
	case SIGINT:
	case SIGHUP:
	case SIGTERM:
		std::cout << "caught a signal=" << sig << std::endl;
		break;
	default:
		std::cerr << "caught an unexpected signal=" << sig << std::endl;
		break;
	}
}

int main(int argc, char *argv[])
{
	std::signal(SIGINT, SigHandler);
	std::signal(SIGHUP, SigHandler);
	std::signal(SIGTERM, SigHandler);
	if (2 != argc)
	{
		std::cerr << "USAGE: " << argv[0] << " /full/pathname/to/config/file>" << std::endl;
		return EXIT_FAILURE;
	}
	// remove pidfile
	remove(g_CFG.GetPidPath().c_str());

	// splash
	std::cout << "Starting mrefd version #" << g_Version << std::endl;

	// and let it run
	if ( g_Reflector.Start(argv[1]) )
	{
		std::cout << "Error starting reflector" << std::endl;
		return EXIT_FAILURE;
	}
	std::cout << "Reflector " << g_CFG.GetCallsign()  << " started and listening" << std::endl;

	// write new pid file
	std::ofstream ofs(g_CFG.GetPidPath(), std::ofstream::out);
	ofs << getpid() << std::endl;
	ofs.close();

	pause(); // wait for any signal

	g_Reflector.Stop();
	std::cout << "Reflector stopped" << std::endl;

	// done
	return EXIT_SUCCESS;
}
