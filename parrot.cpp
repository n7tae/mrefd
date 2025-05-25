//
//  Copyright © 2025 Thomas A. Early, N7TAE
//
// ----------------------------------------------------------------------------
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    with this software.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <string.h>
#include <thread>

#include "packet.h"
#include "newsid.h"
#include "parrot.h"
#include "udpsocket.h"

static CNewStreamID NewSID;

void CParrot::Add(const uint8_t *voice)
{
	if (pos < 250u)
	{
		size_t length = is3200 ? 16 : 8;
		data[pos].resize(length);
		memcpy(data[pos].data(), voice, length);
		pos++;
	}
	lastHeard.Start();
}

void CParrot::Play()
{
	fut = std::async(std::launch::async, &CParrot::playThread, this);
}

void CParrot::playThread()
{
	CPacket pack;
	pack.Initialize(54u, true);
	pack.SetStreamId(NewSID.Make());
	memset(pack.GetDstAddress(), 0xffu, 6);
	src.CodeOut(pack.GetSrcAddress());
	pack.SetFrameType(is3200 ? 0x5u : 0x7u);
	auto clock = std::chrono::steady_clock::now();
	CUdpSocket sock;
	for (size_t fn=0; fn<pos; fn++)
	{
		memcpy(pack.GetVoice(), data[fn].data(), is3200 ? 16 : 8);
		if ((pos - fn) == 1)
			fn |= 0x8000u;
		pack.SetFrameNumber(uint8_t(fn));
		pack.CalcCRC();
		clock = clock + std::chrono::milliseconds(40);
		std::this_thread::sleep_until(clock);
		sock.Send(pack.GetCData(), pack.GetSize(), ip);
		data[fn].clear();
	}
	data.clear();
	isDone = true;
}

bool CParrot::IsDone()
{
	if (isDone)
	{
		if (fut.valid())
			fut.get();
	}
	return isDone;
}
