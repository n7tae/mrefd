/*
 *   Copyright (c) 2022 by Thomas A. Early N7TAE
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

#ifndef NO_DHT
#include <curl/curl.h>
#endif
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <regex>

#include "configure.h"

CConfigure g_CFG;

static inline void split(const std::string &s, char delim, std::vector<std::string> &v)
{
	std::istringstream iss(s);
	std::string item;
	while (std::getline(iss, item, delim))
		v.push_back(item);
}

// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

#ifndef NO_DHT
// callback function writes data to a std::ostream
static size_t data_write(void* buf, size_t size, size_t nmemb, void* userp)
{
	if(userp)
	{
		std::ostream& os = *static_cast<std::ostream*>(userp);
		std::streamsize len = size * nmemb;
		if(os.write(static_cast<char*>(buf), len))
			return len;
	}

	return 0;
}

static CURLcode curl_read(const std::string& url, std::ostream& os, long timeout = 30)
{
	CURLcode code(CURLE_FAILED_INIT);
	CURL* curl = curl_easy_init();

	if(curl)
	{
		if(CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &data_write))
		&& CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L))
		&& CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L))
		&& CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_FILE, &os))
		&& CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout))
		&& CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_URL, url.c_str())))
		{
			code = curl_easy_perform(curl);
		}
		curl_easy_cleanup(curl);
	}
	return code;
}

void CConfigure::CurlAddresses(std::string &ipv4, std::string &ipv6)
{
	std::ostringstream oss;

	curl_global_init(CURL_GLOBAL_ALL);

	if(CURLE_OK == curl_read("https://ipv4.icanhazip.com", oss))
	{
		// Web page successfully written to string
		ipv4.assign(oss.str());
		trim(ipv4);
		oss.str(std::string());
	}

	if(CURLE_OK == curl_read("https://ipv6.icanhazip.com", oss))
	{
		ipv6.assign(oss.str());
		trim(ipv6);
	}

	curl_global_cleanup();

	std::cout << "IPv4=" << ipv4 << " IPv6=" << ipv6 << std::endl;
}
#endif

bool CConfigure::ReadData(const std::string &path)
// returns true on failure
{
	data.port = 17000U;
	data.mcclients = false;
	std::ifstream cfg(path.c_str(), std::ifstream::in);
	if (! cfg.is_open()) {
		std::cerr << path << " was not found!" << std::endl;
		return true;
	}

#ifndef NO_DHT
	std::string ipv4, ipv6;
	CurlAddresses(ipv4, ipv6);
#endif

	std::string line;
	while (std::getline(cfg, line)) {
		trim(line);

		if (0==line.size())
			continue;	// skip empty lines

		if ('#'==line[0])
			continue;	// skip comments

		if (std::string::npos == line.find('='))
		{
			std::cout << "WARNING - No equal sign found: '" << line << "'" << std::endl;
			continue;
		}

		std::vector<std::string> tokens;
		split(line, '=', tokens);

		if (2 > tokens.size())
		{
			std::cout << "WARNING - bad assignment: '" << line << "'" << std::endl;
			continue;
		}

		rtrim(tokens[0]);
		ltrim(tokens[1]);
		const std::string key(tokens[0]);
		const std::string value(tokens[1]);
		if (key.empty() || value.empty())
		{
			std::cout << "WARNING - missing key or value: '" << line << "'" << std::endl;
			continue;
		}

		if (0 == key.compare("Callsign"))
		{
			data.callsign.assign(value);
		}
		else if (0 == key.compare("Modules"))
		{
			for (auto cit=value.begin(); cit!=value.end(); cit++)
			{
				auto c = *cit;
				if (std::islower(c))
					c = std::toupper(c);
				if (std::string::npos == data.mods.find(c))
					data.mods.append(1, c);
			}
		}
		else if (0 == key.compare("EncryptionAllowed"))
		{
			for (auto cit=value.begin(); cit!=value.end(); cit++)
			{
				auto c = *cit;
				if (std::islower(c))
					c = std::toupper(c);
				if (std::string::npos == data.encryptedmods.find(c))
					data.encryptedmods.append(1, c);
			}
		}
		else if (0 == key.compare("IPv4BindAddr"))
		{
			data.ipv4bindaddr.assign(value);
		}
		else if (0 == key.compare("IPv6BindAddr"))
		{
			data.ipv6bindaddr.assign(value);
		}
#ifndef NO_DHT
		else if (0 == key.compare("IPv4ExtAddr"))
		{
			if (value.compare(ipv4))
			{
				std::cout << "WARNING: IPv4ExtAddr '" << value << "' differs from curl result of '" << ipv4 << "'" << std::endl;
			}
			data.ipv4extaddr.assign(value);
		}
		else if (0 == key.compare("IPv6ExtAddr"))
		{
			if (value.compare(ipv6))
			{
				std::cout << "WARNING: IPv6ExtAddr '" << value << "' differs from curl result of '" << ipv6 << "'" << std::endl;
			}
			data.ipv6extaddr.assign(value);
		}
		else if (0 == key.compare("DashboardURL"))
		{
			data.url.assign(value);
		}
		else if (0 == key.compare("EmailAddr"))
		{
			data.emailaddr.assign(value);
		}
		else if (0 == key.compare("Bootstrap"))
		{
			data.bootstrap.assign(value);
		}
		else if (0 == key.compare("Sponsor"))
		{
			data.sponsor.assign(value);
		}
		else if (0 == key.compare("Country"))
		{
			data.country.assign(value.substr(0, 2));
		}
#endif
		else if (0 == key.compare("XmlPath"))
		{
			data.xmlpath.assign(value);
		}
		else if (0 == key.compare("PidPath"))
		{
			data.pidpath.assign(value);
		}
		else if (0 == key.compare("WhitelistPath"))
		{
			data.whitepath.assign(value);
		}
		else if (0 == key.compare("BlacklistPath"))
		{
			data.blackpath.assign(value);
		}
		else if (0 == key.compare("InterlinkPath"))
		{
			data.interlinkpath.assign(value);
		}
		else if (0 == key.compare("DHTSavedNodesPath"))
		{
			data.dhtstatepath.assign(value);
		}
		else if (0 == key.compare("Port"))
		{
			data.port = std::stoul(value);
		}
		else if (0 == key.compare("MultiClient"))
		{
			data.mcclients = IS_TRUE(value[0]);
		}
		else
		{
			std::cout << "WARNING - unknown parameter: '" << line << "'" << std::endl;
		}
	}
	cfg.close();

	////////////////////////////// check the input

	bool rval = false;

	if (data.callsign.empty())
	{
		std::cerr << "Callsign is undefined" << std::endl;
		rval = true;
	}
	else
	{
		auto RefRegEx = std::regex("^M17-([A-Z0-9]){3,3}$", std::regex::extended);
		if (std::regex_match(data.callsign, RefRegEx))
		{
			std::cout << "Callsign='" << data.callsign << "'" << std::endl;
		}
		else
		{
			std::cerr << "ERROR - Malformed callsign: '" << data.callsign << "'" << std::endl;
			rval = true;
		}
	}

	if (data.mods.empty())
	{
		std::cout << "ERROR - no modules defined" << std::endl;
		rval = true;
	}
	else
	{
		std::cout << "Modules='" << data.mods << "'" << std::endl;
	}

	std::cout << "EncryptionAllowed='" << data.encryptedmods << "'" << std::endl;

#ifndef NO_DHT
	if (data.ipv4extaddr.empty())
	{
		data.ipv4extaddr.assign(ipv4);
	}

	if (data.ipv4bindaddr.empty())
	{
		data.ipv4extaddr.clear();
	}
	else
#else
	if (! data.ipv4bindaddr.empty())
#endif
	{
		auto IPv4RegEx = std::regex("^((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\\.){3,3}(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9]){1,1}$", std::regex::extended);

#ifndef NO_DHT
		// there's a binding address
		if (std::regex_match(data.ipv4bindaddr, IPv4RegEx))
		{
			// and it looks okay
			if (data.ipv4extaddr.empty())
			{
				// there's no external address
				std::cerr << "ERROR - IPv4BindAddr removed because there is no IPv4ExtAddr" << std::endl;
				data.ipv4bindaddr.clear();
			}
			else
			{
				// there's an external address
				if (std::regex_match(data.ipv4extaddr, IPv4RegEx))
				{
					std::cout << "IPv4 Bind='" << data.ipv4bindaddr << "; External='" << data.ipv4extaddr << "'" << std::endl;
				}
				else
				{
					// but it's bad
					std::cerr << "ERROR - Malformed IPv4ExtAddress: '" << data.ipv4extaddr << "'" << std::endl;
					data.ipv4bindaddr.clear();
					data.ipv4extaddr.clear();
				}
			}
		}
		else
#else
		if (! std::regex_match(data.ipv4bindaddr, IPv4RegEx))
#endif
		{
			// the binding address is bad
			std::cerr << "ERROR - Malformed IPv4BindAddr: '" << data.ipv4bindaddr << "'" << std::endl;
			data.ipv4bindaddr.clear();
#ifndef NO_DHT
			data.ipv4extaddr.clear();
#endif
		}
	}

#ifndef NO_DHT
	if (data.ipv6extaddr.empty())
	{
		data.ipv6extaddr.assign(ipv6);
	}

	if (data.ipv6bindaddr.empty())
	{
		data.ipv6extaddr.clear();
	}
	else
#else
	if (! data.ipv6bindaddr.empty())
#endif
	{
		auto IPv6RegEx = std::regex("^(([0-9a-fA-F]{1,4}:){7,7}[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,7}:|([0-9a-fA-F]{1,4}:){1,6}(:[0-9a-fA-F]{1,4}){1,1}|([0-9a-fA-F]{1,4}:){1,5}(:[0-9a-fA-F]{1,4}){1,2}|([0-9a-fA-F]{1,4}:){1,4}(:[0-9a-fA-F]{1,4}){1,3}|([0-9a-fA-F]{1,4}:){1,3}(:[0-9a-fA-F]{1,4}){1,4}|([0-9a-fA-F]{1,4}:){1,2}(:[0-9a-fA-F]{1,4}){1,5}|([0-9a-fA-F]{1,4}:){1,1}(:[0-9a-fA-F]{1,4}){1,6}|:((:[0-9a-fA-F]{1,4}){1,7}|:))$", std::regex::extended);

#ifndef NO_DHT
		// there's a binding address
		if (std::regex_match(data.ipv6bindaddr, IPv6RegEx))
		{
			// and it looks okay
			if (data.ipv6extaddr.empty())
			{
				// there's no external address
				std::cerr << "ERROR - IPv6BindAddr removed because there is no IPv6ExtAddr" << std::endl;
				data.ipv6bindaddr.clear();
			}
			else
			{
				// there's an external address
				if (std::regex_match(data.ipv6extaddr, IPv6RegEx))
				{
					std::cout << "IPv6 Bind='" << data.ipv6bindaddr << "; External='" << data.ipv6extaddr << "'" << std::endl;
				}
				else
				{
					// but it's bad
					std::cerr << "ERROR - Malformed IPv6ExtAddress: '" << data.ipv6extaddr << "'" << std::endl;
					data.ipv6bindaddr.clear();
					data.ipv6extaddr.clear();
				}
			}
		}
		else
#else
		if (! std::regex_match(data.ipv6bindaddr, IPv6RegEx))
#endif
		{
			// the binding address is bad
			std::cerr << "ERROR - Malformed IPv6BindAddr: '" << data.ipv6bindaddr << "'" << std::endl;
			data.ipv6bindaddr.clear();
#ifndef NO_DHT
			data.ipv6extaddr.clear();
#endif
		}
	}

	// there has to be an address
	if (data.ipv4bindaddr.empty() && data.ipv6bindaddr.empty())
	{
		std::cerr << "ERROR - There is no IPv4 or 6 binding address" << std::endl;
		rval = true;
	}

#ifndef NO_DHT
	if (data.url.empty())
	{
		std::cerr << "ERROR - no dashboard URL" << std::endl;
		rval = true;
	}
	else
	{
		auto URLRegEx = std::regex("^https?://.+$", std::regex::extended);
		if (std::regex_match(data.url, URLRegEx))
		{
			std::cout << "DashboardURL='" << data.url << "'" << std::endl;
		}
		else
		{
			std::cerr << "ERROR - DashboaardURL malformed" << std::endl;
			rval = true;
		}
	}

	if (data.emailaddr.empty())
	{
		std::cerr << "ERROR - no EmailAddr" << std::endl;
		rval = true;
	}
	else
	{
		std::cout << "EmailAddr='" << data.emailaddr << "'" << std::endl;
	}

	if (data.bootstrap.empty())
	{
		std::cerr << "ERROR - no Bootstrap specified" << std::endl;
		rval = true;
	}
	else
	{
		std::cout << "Bootstrap='" << data.bootstrap << "'" << std::endl;
	}

	if (!data.sponsor.empty())
	{
		std::cout << "Sponsor='" << data.sponsor << "'" << std::endl;
	}

	if (data.country.empty())
	{
		std::cerr << "ERROR - no Country" << std::endl;
	}
	else
	{
		for (auto &c : data.country)
		{
			if (std::islower(c))
				c = std::toupper(c);
		}
		std::cout << "Country='" << data.country << "'" << std::endl;
	}
#endif

	if (data.xmlpath.empty())
	{
		std::cerr << "ERROR - XMLPath is empty" << std::endl;
		rval = true;
	}
	else
	{
		std::cout << "XmlPath='" << data.xmlpath << "'" << std::endl;
	}

	if (data.pidpath.empty())
	{
		std::cerr << "ERROR - PidPath is empty" << std::endl;
		rval = true;
	}
	else
	{
		std::cout << "PidPath='" << data.pidpath << "'" << std::endl;
	}

	if (data.whitepath.empty())
	{
		std::cerr << "ERROR - WhitelistPath is empty" << std::endl;
		rval = true;
	}
	else
	{
		std::cout << "WhitelistPath='" << data.whitepath << "'" << std::endl;
	}

	if (data.blackpath.empty())
	{
		std::cerr << "ERROR - BlacklistPath is empty" << std::endl;
		rval = true;
	}
	else
	{
		std::cout << "BlacklistPath='" << data.blackpath << "'" << std::endl;
	}

	if (data.interlinkpath.empty())
	{
		std::cerr << "ERROR - InterlinkPath is empty" << std::endl;
		rval = true;
	}
	else
	{
		std::cout << "InterlinkPath='" << data.interlinkpath << "'" << std::endl;
	}

	if (data.dhtstatepath.empty())
	{
		std::cerr << "ERROR - no DHT network state path specified" << std::endl;
		rval = true;
	}
	else
	{
		std::cout << "DHTSavedNodesPath='" << data.dhtstatepath << "'" << std::endl;
	}

	if (49000U < data.port || data.port < 4096U)
	{
		std::cerr << "ERROR - Port is out of range" << std::endl;
		rval = true;
	}
	else
	{
		std::cout << "Port=" << data.port << std::endl;
	}

	std::cout << "MultiClient=" << (data.mcclients ? "true" : "false") << std::endl;

	return rval;
}

bool CConfigure::IsEncyrptionAllowed(const char mod)
{
	 if (std::string::npos != data.encryptedmods.find(mod))
	 	return true;
	else
		return false;
}
