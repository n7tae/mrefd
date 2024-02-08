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

#pragma once

#include <string>

#define IS_TRUE(a) ((a)=='t' || (a)=='T' || (a)=='1')

using CFGDATA = struct CFGData_struct {
	std::string callsign, mods, encryptedmods;
	std::string ipv4bindaddr, ipv6bindaddr;
#ifndef NO_DHT
	std::string ipv4extaddr, ipv6extaddr;
	std::string url, emailaddr, bootstrap;
	std::string sponsor, country;
	std::string dhtstatepath;
#endif
	std::string pidpath, xmlpath, whitepath, blackpath, interlinkpath;
	unsigned long port;
	bool mcclients;
};

class CConfigure
{
public:
	CConfigure() {}
	bool ReadData(const std::string &path);
	bool IsValidModule(char c) const { return std::string::npos != data.mods.find(c); }
	bool IsEncyrptionAllowed(const char mod);

	const std::string &GetCallsign()      const { return data.callsign;      }
	const std::string &GetModules()       const { return data.mods;          }
	const std::string &GetEncryptedMods() const { return data.encryptedmods; }
	const std::string &GetIPv4BindAddr()  const { return data.ipv4bindaddr;  }
	const std::string &GetIPv6BindAddr()  const { return data.ipv6bindaddr;  }
#ifndef NO_DHT
	const std::string &GetIPv4ExtAddr()   const { return data.ipv4extaddr;   }
	const std::string &GetIPv6ExtAddr()   const { return data.ipv6extaddr;   }
	const std::string &GetURL()           const { return data.url;           }
	const std::string &GetEmailAddr()     const { return data.emailaddr;     }
	const std::string &GetBootstrap()     const { return data.bootstrap;     }
	const std::string &GetSponsor()       const { return data.sponsor;       }
	const std::string &GetCountry()       const { return data.country;       }
	const std::string &GetDHTStatePath()  const { return data.dhtstatepath;  }
#endif
	const std::string &GetXmlPath()       const { return data.xmlpath;       }
	const std::string &GetPidPath()       const { return data.pidpath;       }
	const std::string &GetWhitePath()     const { return data.whitepath;     }
	const std::string &GetBlackPath()     const { return data.blackpath;     }
	const std::string &GetInterlinkPath() const { return data.interlinkpath; }
	unsigned long      GetPort()          const { return data.port;          }
	bool               GetMCClients()     const { return data.mcclients;     }

private:
#ifndef NO_DHT
	void CurlAddresses(std::string &v4, std::string &v6);
#endif
	CFGDATA data;
};
