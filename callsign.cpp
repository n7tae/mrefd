/*
 *   Copyright (c) 2020 by Thomas A. Early N7TAE
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

#include <iostream>

#include "configure.h"
#include "callsign.h"

#define M17CHARACTERS " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-/."

CCallsign::CCallsign()
{
	memset(cs, 0, sizeof(cs));
	memset(code, 0, sizeof(code));
}

CCallsign::CCallsign(const std::string &callsign)
{
	CSIn(callsign);
}

CCallsign::CCallsign(const uint8_t *in)
{
	CodeIn(in);
}

void CCallsign::CSIn(const std::string &callsign)
{
	const std::string m17_alphabet(M17CHARACTERS);
	memset(cs, 0, sizeof(cs));
	memcpy(cs, callsign.c_str(), (callsign.size()<10) ? callsign.size() : 9);	// no more than 9 chars
	uint64_t encoded = 0;
	for( int i=int(callsign.size()-1); i>=0; i-- ) {
		auto pos = m17_alphabet.find(cs[i]);
		if (pos == std::string::npos) {
			pos = 0;
		}
		cs[i] = m17_alphabet[pos];	// replace with valid character
		encoded *= 40;
		encoded += pos;
	}

	// strip trailing spaces (just to be nice!)
	auto len = strlen(cs);
	while ((' ' == cs[len-1]) && len)
		cs[--len] = 0;

	// fill in the encoded value
	for (int i=0; i<6; i++) {
		code[i] = (encoded >> (8*(5-i)) & 0xFFU);
	}
}

void CCallsign::CodeIn(const uint8_t *in)
{
	const std::string m17_alphabet(M17CHARACTERS);
	memset(cs, 0, 10);
	uint64_t coded = in[0];
	for (int i=1; i<6; i++)
		coded = (coded << 8) | in[i];
	if (coded > 0xee6b27ffffffu) {
		std::cerr << "Callsign code is too large, 0x" << std::hex << coded << std::dec << std::endl;
		return;
	}
	memcpy(code, in, 6);
	int i = 0;
	while (coded) {
		cs[i++] = m17_alphabet[coded % 40];
		coded /= 40;
	}
}

const std::string CCallsign::GetCS(unsigned len) const
{
	if (len > 9)
		len = 9;
	std::string rval(cs);
	rval.resize(len, ' ');
	return rval;
}

char CCallsign::GetModule() const
{
	if (cs[8])
		return cs[8];
	else
		return ' ';
}

bool CCallsign::HasSameCallsign(const CCallsign &call) const
{
	return (this->GetCS(8) == call.GetCS(8));
}

bool CCallsign::HasValidModule() const
{
	auto i = GetModule() - 'A';
	if (i >= 0)
	{
		if (i < NB_OF_MODULES)
			return true;
	}
	return false;
}

bool CCallsign::operator==(const CCallsign &rhs) const
{
	return (0 == memcmp(code, rhs.code, 6));
}

bool CCallsign::operator!=(const CCallsign &rhs) const
{
	return (0 != memcmp(code, rhs.code, 6));
}

bool CCallsign::HasSameCallsignWithWildcard(const CCallsign &callsign) const
{
	bool same = true;
	bool done = false;

	for ( unsigned i = 0; (i < sizeof(cs)) && same && !done; i++ )
	{
		if ( !(done = ((cs[i] == '*') || (callsign.cs[i] == '*'))) )
		{
			same &= (cs[i] == callsign.cs[i]);
		}
	}
	return same;
}

void CCallsign::SetModule(char m)
{
	std::string call(cs);
	call.resize(8, ' ');
	call.append(1, m);
	CSIn(call);
}

std::ostream &operator<<(std::ostream &stream, const CCallsign &call)
{
	stream << call.cs;
	return stream;
}
