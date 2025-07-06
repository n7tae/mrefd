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
#include <sstream>

#include "packet.h"
#include "newsid.h"
#include "parrot.h"
#include "udpsocket.h"
#include "crc.h"

static CNewStreamID NewSID;

void CStreamParrot::Add(const CPacket &pack)
{
	if (data.size() < 500u)
	{
		size_t length = is3200 ? 16 : 8;
		auto voice = pack.GetCVoice();
		data.emplace_back(voice, voice+length);
	}
	lastHeard.Start();
}

void CStreamParrot::Play()
{
	fut = std::async(std::launch::async, &CStreamParrot::playThread, this);
}

void CStreamParrot::playThread()
{
	state = EParrotState::play;
	CPacket pack;
	memcpy(pack.GetData(), "M17 ", 4);
	pack.Initialize(54u, true);
	pack.SetStreamId(NewSID.Make());
	memset(pack.GetDstAddress(), 0xffu, 6);
	src.CodeOut(pack.GetSrcAddress());
	pack.SetFrameType(frameType);
	auto clock = std::chrono::steady_clock::now();
	CUdpSocket sock;
	size = data.size();
	for (size_t n=0; n<size; n++)
	{
		memcpy(pack.GetVoice(), data[n].data(), is3200 ? 16 : 8);
		pack.SetFrameNumber((size-n == 1) ? 0x8000u + n : n);
		pack.CalcCRC();
		clock = clock + std::chrono::milliseconds(40);
		std::this_thread::sleep_until(clock);
		client->SendPacket(pack);
		data[n].clear();
	}
	data.clear();
	state = EParrotState::done;
}

void CPacketParrot::Add(const CPacket &pack)
{
	packet.SetSize(pack.GetSize());
	packet.SetType(false);
	memcpy(packet.GetData(), pack.GetCData(), pack.GetSize());
}

void CPacketParrot::Play()
{
	fut = std::async(std::launch::async, &CPacketParrot::returnPacket, this);
}

void CPacketParrot::returnPacket()
{
	CCRC crc;
	state = EParrotState::play;
	const auto crc1 = packet.GetCRC(true);
	const auto cal1 = crc.CalcCRC(packet.GetCData()+4, 28);
	const auto crc2 = packet.GetCRC(false);
	const auto cal2 = crc.CalcCRC(packet.GetCData()+34, packet.GetSize()-36);

	const bool firstcrc = crc1 == cal1;
	const bool secondcrc = crc2 == cal2;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	memset(packet.GetDstAddress(), 0xffu, 6);
	if (firstcrc and secondcrc)
	{
		packet.CalcCRC();
		client->SendPacket(packet);
	}
	else
	{
		std::stringstream ss;
		ss << std::hex << std::showbase << "There were problems with your packet:" << std::endl;
		if (not firstcrc)
			ss << "  The LSF CRC was wrong." << std::endl <<"    You had " << crc1 << " but it should have been " << cal1 << std::endl;
		if (not secondcrc)
			ss << "  The Payload CRC was wrong." << std::endl << "    You had " << crc2 << " but it should have been " << cal2 << std::endl;
		
		const std::string str = ss.str();
		const auto l = str.length();
		packet.SetFrameType(frameType);
		packet.GetData()[34] = 0x5u; // SMS type
		memcpy(packet.GetData()+35, str.c_str(), l);
		size_t tps = 4 + 30 + 3 + l;
		packet.SetSize(tps);
		packet.SetType(false);
		packet.CalcCRC();
		for (; tps < MAX_PACKET_SIZE; tps++) packet.GetData()[tps] = 0u;
		client->SendPacket(packet);
	}
	state = EParrotState::done;
}
