//
//  mrefd
//
//  Created by Jean-Luc Deltombe (LX3JL) on 01/11/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2022-2025 Thomas A. Early, N7TAE
//
// ----------------------------------------------------------------------------
//    This file is part of mrefd.
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
//    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include "defines.h"
#include "protocol.h"
#include "clients.h"
#include "reflector.h"
#include "gatekeeper.h"
#include "configure.h"
#include "interlinks.h"

extern CConfigure g_CFG;
extern CGateKeeper g_GateKeeper;
extern CReflector g_Reflector;
extern CInterlinks g_Interlinks;

////////////////////////////////////////////////////////////////////////////////////////
// constructor

CProtocol::CProtocol() : keep_running(true), publish(true)
{
	peerRegEx = std::regex("^M17-[A-Z0-9]{3,3}( [A-Z])?$", std::regex::extended);
	clientRegEx = std::regex("^[0-9]?[A-Z]{1,2}[0-9]{1,2}[A-Z]{1,4}([ -/\\.].*)?$", std::regex::extended);
	lstnRegEx = std::regex("^[0-9]?[A-Z][A-Z0-9]{2,8}$", std::regex::extended);
}

////////////////////////////////////////////////////////////////////////////////////////
// destructor

CProtocol::~CProtocol()
{
	Close();
}

////////////////////////////////////////////////////////////////////////////////////////
// initialization

bool CProtocol::Initialize(const uint16_t port, const std::string &strIPv4, const std::string &strIPv6)
{
	// init reflector apparent callsign
	m_ReflectorCallsign = g_CFG.GetCallsign();

	// reset stop flag
	keep_running = true;

	// create our sockets
	if (! strIPv4.empty())
	{
		CIp ip4(AF_INET, port, strIPv4.c_str());
		if ( ip4.IsSet() )
		{
			if (! m_Socket4.Open(ip4))
				return false;
		}
		std::cout << "Listening on " << ip4 << std::endl;
	}

	if (! strIPv6.empty())
	{
		CIp ip6(AF_INET6, port, strIPv6.c_str());
		if ( ip6.IsSet() )
		{
			if (! m_Socket6.Open(ip6))
			{
				m_Socket4.Close();
				return false;
			}
			std::cout << "Listening on " << ip6 << std::endl;
		}
	}

	// set up the Receive function pointer
	if (strIPv4.empty())
	{
		Receive = &CProtocol::Receive6;
	}
	else
	{
		if (strIPv6.empty())
		{
			Receive = &CProtocol::Receive4;
		}
		else
		{
			Receive = &CProtocol::ReceiveDS;
		}
	}

	for (const auto &mod : g_CFG.GetRefMods().GetModules())
	{
		m_streamMap[mod] = std::make_unique<CPacketStream>();
	}

	try {
		m_Future = std::async(std::launch::async, &CProtocol::Thread, this);
	}
	catch (const std::exception &e)
	{
		std::cerr << "Could not start protocol on port " << port << ": " << e.what() << std::endl;
		m_Socket4.Close();
		m_Socket6.Close();
		return false;
	}

	// update time
	m_LastKeepaliveTime.Start();
	m_LastPeersLinkTime.Start();

	// done
	return true;
}

void CProtocol::Thread()
{
	while (keep_running)
	{
		Task();
	}
}
////////////////////////////////////////////////////////////////////////////////////////
// task

void CProtocol::Task(void)
{
	CIp       ip;
	char      mod;
	char      mods[27];
	CCallsign cs;
	CPacket   pack;

	// any incoming packet ?
	auto len = (*this.*Receive)(pack.GetData(), ip, 20);

 	if (len < 0)
		len = 0;

	switch (len)
	{
	default:
		if (len > ssize_t(sizeof(SInterConnect)))
		{
			// we're going to need the module right away, so we'll find out who is sending this packet
			auto client = g_Reflector.GetClients()->FindClient(ip);
			g_Reflector.ReleaseClients();
			if (client)
			{
				mod = client->GetReflectorModule();
				pack.SetFromType(client->GetClientType());
				// sets the packet type and size based on the magic value and the `len` value
				// NOTE: this could be a packet from a simple client or another reflector
				// check that the source callsign matches a licensed ham callsign,
				// check that the desination is a configured module callsign, for example 'M17-M17 C'
				// if the packet is a stream packet, and is encrypted, make sure the module is configured for it.
				if ( IsValidPacket(pack, len, mod) )
				{
					// make sure the SRC callsign is not blacklisted
					const CCallsign src(pack.GetCSrcAddress());
					if (g_GateKeeper.MayTransmit(src, ip))
					{
						// might open a new stream if it's the first packet
						// if there is a problem, return false
						if (OnPacketIn(pack, client))
						{
							SendToAllClients(pack, client, mod);
							if (pack.IsStreamPacket() and pack.IsLastPacket())
							{
								CloseStream(mod); // so this only closes streams, PM packets time out the PacketStream
							}
						}				
					}
					else if (pack.IsStreamPacket())
					{
						// this voicestream is blocked, so
						if (pack.GetFrameNumber() & 0x8000u)
						{
							// when the stream closes, log it.
							std::cout << "Blocked voice stream from " << src << " at " << ip << std::endl;
						}
					}
					else
					{
						// here is a blocked PM packet
						std::cout << "Blocked Packet from " << src <<  " at " << ip << std::endl;
					}
				}
			}
		}
		break;
	case sizeof(SInterConnect):
		if (IsValidInterlinkConnect(pack.GetCData(), cs, mods))
		{
			std::cout << "CONN packet from " << cs <<  " at " << ip << " to module(s) " << mods << std::endl;

			// callsign authorized?
			if ( g_GateKeeper.MayLink(cs, ip) )
			{
				SInterConnect ackn;
				// acknowledge the request
				EncodeInterlinkAckPacket(ackn, mods);
				Send(ackn.magic, sizeof(SInterConnect), ip);
			}
			else
			{
				// deny the request
				EncodeInterlinkNackPacket(pack.GetData());
				Send(pack.GetCData(), 10, ip);
			}

		}
		else if (IsValidInterlinkAcknowledge(pack.GetCData(), cs, mods))
		{
			std::cout << "ACQN packet from " << cs << " at " << ip << " on module(s) " << mods << std::endl;

			// callsign authorized?
			if ( g_GateKeeper.MayLink(cs, ip) )
			{
				// already connected ?
				auto peers = g_Reflector.GetPeers();
				if ( nullptr == peers->FindPeer(cs, ip) )
				{
					auto item = g_Interlinks.Find(cs.GetCS());
					if (item)
					{
						auto type = item->IsNotLegacy() ? EClientType::reflector : EClientType::legacy;
						// create the new peer
						// this also create one client per module
						if (AF_INET6 == ip.GetFamily())
							peers->AddPeer(std::make_shared<CPeer>(cs, ip, type, mods, m_Socket6));
						else
							peers->AddPeer(std::make_shared<CPeer>(cs, ip, type, mods, m_Socket4));
						publish = true;
					}
					else
					{
						std::cerr << "ERROR: gotn ACKN packet from " << cs.GetCS() << " but could not find the interlink item!" << std::endl;
					}
				}
				g_Reflector.ReleasePeers();
			}
		}
		break;
	case 11:
		if ( IsValidConnect(pack.GetCData(), cs, mod) )
		{
			bool isLstn = (0 == memcmp(pack.GetCData(), "LSTN", 4));

			std::cout << "Connect packet for module " << mod << " from " << cs << " at " << ip << (isLstn ? " as listen-only" : "") << std::endl;

			// callsign authorized?
			if ( g_GateKeeper.MayLink(cs, ip) )
			{
				// valid module ?
				if ( g_CFG.IsValidModule(mod) )
				{
					// acknowledge a normal request from a repeater/hot-spot/mvoice
					EncodeConnectAckPacket(pack.GetData());
					Send(pack.GetCData(), 4, ip);

					// create the client and append
					if (isLstn) {
						if (g_CFG.GetRefMods().GetEModules().find(mod) != std::string::npos && not g_CFG.GetSWLEncryptedMods()) 
						{
							std::cout << "SWL Node " << cs << " is not allowed to connect to encrypted Module '" << mod << "'" << std::endl;

							// deny the request
							EncodeConnectNackPacket(pack.GetData());
							Send(pack.GetCData(), 4, ip);
						} else {
							if(AF_INET6 == ip.GetFamily())
								g_Reflector.GetClients()->AddClient(std::make_shared<CClient>(cs, ip, EClientType::listenonly, mod, m_Socket6));
							else
								g_Reflector.GetClients()->AddClient(std::make_shared<CClient>(cs, ip, EClientType::listenonly, mod, m_Socket4));
						}
					} else {
						if (AF_INET6 == ip.GetFamily())
							g_Reflector.GetClients()->AddClient(std::make_shared<CClient>(cs, ip, EClientType::simple, mod, m_Socket6));
						else
							g_Reflector.GetClients()->AddClient(std::make_shared<CClient>(cs, ip, EClientType::simple, mod, m_Socket4));
					}
					g_Reflector.ReleaseClients();
				}
				else
				{
					std::cout << "Node " << cs << " connect attempt on non-existing module '" << mod << "'" << std::endl;

					// deny the request
					EncodeConnectNackPacket(pack.GetData());
					Send(pack.GetCData(), 4, ip);
				}
			}
			else
			{
				// deny the request
				EncodeConnectNackPacket(pack.GetData());
				Send(pack.GetCData(), 4, ip);
			}
		}
		break;
	case 10:
		if ( IsValidKeepAlive(pack.GetCData(), cs) )
		{
			if (cs.GetCS(4).compare("M17-")) {
				// find all clients with that callsign & ip and keep them alive
				auto clients = g_Reflector.GetClients();
				auto it = clients->begin();
				SPClient client = nullptr;
				while (nullptr != (client = clients->FindNextClient(cs, ip, it)))
				{
					client->Alive();
				}
				g_Reflector.ReleaseClients();
			}
			else
			{
				// find peer
				auto peers = g_Reflector.GetPeers();
				auto peer = peers->FindPeer(ip);
				if ( peer )
				{
					// keep it alive
					peer->Alive();
				}
				g_Reflector.ReleasePeers();
			}
		}
		else if ( IsValidDisconnect(pack.GetCData(), cs) )
		{
			std::cout << "Disconnect packet from " << cs << " at " << ip << std::endl;
			if (cs.GetCS(4).compare("M17-")) {
				// find the regular client & remove it
				auto clients = g_Reflector.GetClients();
				auto client = clients->FindClient(ip);
				if ( client != nullptr )
				{
					// ack disconnect packet
					EncodeDisconnectedPacket(pack.GetData());
					Send(pack.GetCData(), 4, ip);
					// and remove it
					clients->RemoveClient(client);
				}
				g_Reflector.ReleaseClients();
			}
			else
			{
				// find the peer and remove it
				auto peers = g_Reflector.GetPeers();
				auto peer = peers->FindPeer(ip);
				if ( peer )
				{
					// remove it from reflector peer list
					// this also remove all peer's clients from reflector client list
					// and delete them
					peers->RemovePeer(peer);
				}
				g_Reflector.ReleasePeers();
			}
		}
		else if ( IsValidNAcknowledge(pack.GetCData(), cs))
		{
			std::cout << "NACK packet received from " << cs << " at " << ip << std::endl;
		}
		break;
	}

	// Now we do all the other maintenance

	// handle end of streaming timeout
	CheckStreamsTimeout();

	// keep alive
	if ( m_LastKeepaliveTime.Time() > M17_KEEPALIVE_PERIOD )
	{
		// handle keep alives
		HandleKeepalives();

		// update time
		m_LastKeepaliveTime.Start();
	}

	// peer connections
	if ( m_LastPeersLinkTime.Time() > M17_RECONNECT_PERIOD )
	{
		// handle remote peers connections
		HandlePeerLinks();

		// update time
		m_LastPeersLinkTime.Start();
	}
}

void CProtocol::Close(void)
{
	keep_running = false;
	if ( m_Future.valid() )
	{
		m_Future.get();
	}
	m_streamMap.clear();
	m_Socket4.Close();
	m_Socket6.Close();
}

////////////////////////////////////////////////////////////////////////////////////////
// stream handle helpers

CPacketStream *CProtocol::GetStream(CPacket &packet, const SPClient client)
{
	// this is only for stream mode packet
	if (not packet.IsStreamPacket())
		return nullptr;
	// get the stream based on the DST module
	const auto mod = client->GetReflectorModule();
	const auto pit = m_streamMap.find(mod);
	// does the dst module exist?
	if (m_streamMap.end() == pit)
	{
		#ifdef DEBUG
		const CCallsign dst(packet.GetCDstAddress());
		const CCallsign src(packet.GetCSrcAddress());
		std::cout << "Bad incoming packet on module '" << mod << "' to " << dst.GetCS() << " from " << src.GetCS() << std::endl;
		#endif
		return nullptr;
	}
	if (pit->second->IsOpen())
	{
		// the stream is opened. do the SIDs match?
		if (pit->second->GetPacketStreamId() == packet.GetStreamId())
		{
			return pit->second.get();
		}
	}
	// done
	return nullptr;
}

void CProtocol::CheckStreamsTimeout(void)
{
	// check the packetstreams for each module
	for (auto &it : m_streamMap)
	{
		// time out ?
		if (it.second->IsExpired())
			CloseStream(it.first);
	}

	// check each item in the parrot map
	for (auto pit = parrotMap.begin(); pit != parrotMap.end();)
	{
		switch (pit->second->GetState())
		{
		case EParrotState::record:
			if (pit->second->IsExpired())
			{
				std::cout << "Parrot stream from " << pit->second->GetSRC() << " timed out! Playing..." << std::endl;
				pit->second->Play();
			}
			pit++;
			break;
		case EParrotState::done:
			if (pit->second->IsStream())
			{
				auto psp = static_cast<CStreamParrot *>(pit->second.get());
				std::cout << psp->GetSize() << " packet parrot stream from " << psp->GetSRC() << " played back to " << pit->first->GetCallsign() << " at " << pit->first->GetIp() << std::endl;
			}
			else
			{
				std::cout << "Parrot packet from " << pit->second->GetSRC() << " played back to " << pit->first->GetCallsign() << " at " << pit->first->GetIp() << std::endl;
			}
			pit->second->Quit();        // get() the future
			pit->second.reset();        // destroy the parrot object
			pit = parrotMap.erase(pit); // remove the map std::pair, incrementing the pointer
			break;
		default:
			break;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// syntax helper

bool CProtocol::IsNumber(char c) const
{
	return ((c >= '0') && (c <= '9'));
}

bool CProtocol::IsLetter(char c) const
{
	return ((c >= 'A') && (c <= 'Z'));
}

bool CProtocol::IsSpace(char c) const
{
	return (c == ' ');
}

////////////////////////////////////////////////////////////////////////////////////////
// Receivers

ssize_t CProtocol::Receive6(uint8_t *buf, CIp &ip, int time_ms)
{
	return m_Socket6.Receive(buf, ip, time_ms);
}

ssize_t CProtocol::Receive4(uint8_t *buf, CIp &ip, int time_ms)
{
	return m_Socket4.Receive(buf, ip, time_ms);
}

ssize_t CProtocol::ReceiveDS(uint8_t *buf, CIp &ip, int time_ms)
{
	auto fd4 = m_Socket4.GetSocket();
	auto fd6 = m_Socket6.GetSocket();

	if (fd4 < 0)
	{
		if (fd6 < 0)
			return false;
		return m_Socket6.Receive(buf, ip, time_ms);
	}
	else if (fd6 < 0)
		return m_Socket4.Receive(buf, ip, time_ms);

	fd_set fset;
	FD_ZERO(&fset);
	FD_SET(fd4, &fset);
	FD_SET(fd6, &fset);
	int max = (fd4 > fd6) ? fd4 : fd6;
	struct timeval tv;
	tv.tv_sec = time_ms / 1000;
	tv.tv_usec = (time_ms % 1000) * 1000;

	auto rval = select(max+1, &fset, 0, 0, &tv);
	if (rval <= 0)
	{
		if (rval < 0)
			std::cerr << "ReceiveDS select error: " << strerror(errno) << std::endl;
		return 0;
	}

	if (FD_ISSET(fd4, &fset))
		return m_Socket4.ReceiveFrom(buf, ip);
	else
		return m_Socket6.ReceiveFrom(buf, ip);
}

////////////////////////////////////////////////////////////////////////////////////////
// dual stack sender

void CProtocol::Send(const uint8_t *buf, size_t size, const CIp &Ip) const
{
	switch (Ip.GetFamily())
	{
	case AF_INET:
		m_Socket4.Send(buf, size, Ip);
		break;
	case AF_INET6:
		m_Socket6.Send(buf, size, Ip);
		break;
	default:
		std::cerr << "ERROR: wrong family: " << Ip.GetFamily() << std::endl;
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// queue helper

void CProtocol::SendToAllClients(CPacket &packet, const SPClient &txclient, const char mod)
{
	#ifdef DEBUG
	if (not packet.IsStreamPacket())
	{
		Dump("Incoming SendToAllClients() PM packet:", packet.GetCData(), packet.GetSize());
		std::cout << "DST=" << CCallsign(packet.GetCDstAddress()) << " SRC=" << CCallsign(packet.GetCSrcAddress()) << std::endl;
	}
	#endif
	// this might be going to a legacy reflector
	if (packet.IsStreamPacket())
	{
		const bool b = true;
		packet.GetData()[54] = uint8_t(b);
	}
	// push it to all our clients linked to the module and who is not streaming in
	SPClient client = nullptr;
	auto clients = g_Reflector.GetClients();
	auto it = clients->begin();
	while (nullptr != (client = clients->FindNextClient(mod, it)))
	{
		// he not TXing and he is not doing a parrot
		if ((client != txclient) and (parrotMap.end() == parrotMap.find(client)))
		{
			const auto ct = client->GetClientType();

			if (EClientType::reflector == ct or EClientType::legacy == ct)
			{
				// the client is a reflector
				if (EClientType::simple == packet.GetFromType())
				{
					// only send it if from a client
					client->SendPacket(packet);
				}
			}
			else 
			{
				// this client is not a reflector
				client->SendPacket(packet);
			}
		}
	}
	g_Reflector.ReleaseClients();
}

////////////////////////////////////////////////////////////////////////////////////////
// keepalive helpers

void CProtocol::HandleKeepalives(void)
{
	uint8_t keepalive[10];
	EncodeKeepAlivePacket(keepalive);

	// iterate on clients
	auto clients = g_Reflector.GetClients();
	auto it = clients->begin();
	SPClient client;
	while ( nullptr != (client = clients->FindNextClient(it)) )
	{
		// don't ping reflector modules, we'll do each interlinked refectors after this while loop
		if (0 == client->GetCallsign().GetCS(4).compare("M17-"))
			continue;

		// send keepalive
		Send(keepalive, 10, client->GetIp());

		// client busy ?
		if ( client->IsTransmitting() )
		{
			// yes, just tickle it
			client->Alive();
		}
		// otherwise check if still with us
		else if ( not client->IsAlive() )
		{
			const auto type = client->GetClientType();
			if (EClientType::simple == type or EClientType::listenonly == type)
			{
				// no, disconnect
				uint8_t disconnect[10];
				EncodeDisconnectPacket(disconnect, client->GetReflectorModule());
				Send(disconnect, 10, client->GetIp());

				// remove it
				std::cout << "Client " << client->GetCallsign() << " keepalive timeout" << std::endl;
				clients->RemoveClient(client);
			}
			g_Reflector.ReleasePeers();
		}

	}
	g_Reflector.ReleaseClients();

	// iterate on peers
	auto peers = g_Reflector.GetPeers();
	auto pit = peers->begin();
	std::shared_ptr<CPeer> peer;
	while ( nullptr != (peer = peers->FindNextPeer(pit)) )
	{
		// send keepalive
		Send(keepalive, 10, peer->GetIp());

		// client busy ?
		if ( peer->IsTransmitting() )
		{
			// yes, just tickle it
			peer->Alive();
		}
		// otherwise check if still with us
		else if ( not peer->IsAlive() )
		{
			// no, disconnect
			uint8_t disconnect[10];
			EncodeDisconnectPacket(disconnect, 0);
			Send(disconnect, 10, peer->GetIp());

			// remove it
			std::cout << "Peer " << peer->GetCallsign() << " keepalive timeout" << std::endl;
			peers->RemovePeer(peer);
		}
	}
	g_Reflector.ReleasePeers();
}

////////////////////////////////////////////////////////////////////////////////////////
// Peers helpers

void CProtocol::HandlePeerLinks(void)
{
	// get the list of peers
	g_Interlinks.Lock();
	auto peers = g_Reflector.GetPeers();

	// check if all our connected peers are still listed in mrefd.interlink
	// if not, disconnect
	auto pit = peers->begin();
	std::shared_ptr<CPeer>peer = nullptr;
	while ( (peer = peers->FindNextPeer(pit)) != nullptr )
	{
		const auto cs = peer->GetCallsign().GetCS();
		if ( nullptr == g_Interlinks.Find(cs) )
		{
			uint8_t buf[10];
			// send disconnect packet
			EncodeDisconnectPacket(buf, 0);
			Send(buf, 10, peer->GetIp());
			std::cout << "Sent disconnect packet to M17 peer " << cs << " at " << peer->GetIp() << std::endl;
			// remove client
			peers->RemovePeer(peer);
			publish = true;
		}
	}

	// check if all ours peers listed in mrefd.interlink are connected
	// if not, connect or reconnect
	for ( auto it=g_Interlinks.begin(); it!=g_Interlinks.end(); it++ )
	{
		auto &item = it->second;
		const auto cs = item->GetCallsign().GetCS();
		if (item->GetIp().IsSet())
		{
			if (nullptr == peers->FindPeer(item->GetCallsign()) and item->GetIp().IsSet())
			{
				// send connect packet to re-initiate peer link
				SInterConnect connect;
				const auto mods = item->GetReqMods();
				EncodeInterlinkConnectPacket(connect, mods);
				Send(connect.magic, sizeof(SInterConnect), item->GetIp());
				std::cout << "Sent connect packet to M17 peer " << item->GetCallsign() << " @ " << item->GetIp() << " for module(s) " << mods << std::endl;
			}
		}
		else
		{
		#ifdef NO_DHT
			std::cerr << "ERROR: " << cs << " doesn't have a vaild IP address!" << std::endl;
		#else
			g_Reflector.GetDHTConfig(cs);
		#endif
		}
	}

	g_Reflector.ReleasePeers();
	g_Interlinks.Unlock();

#ifndef NO_DHT
	if (publish)
	{
		g_Reflector.PutDHTPeers();
		publish = false;
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////////////
// streams helpers

// returns true if the packet is ready for distribution
bool CProtocol::OnPacketIn(CPacket &packet, const SPClient client)
{
	// if the packet dst looks like an M17 reflector, change the dst to @ALL
	CCallsign dst(packet.GetCDstAddress());
	const auto cs = dst.GetCS();
	if (0 == cs.compare(0, 4, "M17-"))
	{
		dst.CSIn("@ALL");
		dst.CodeOut(packet.GetDstAddress());
		packet.CalcCRC();
	}
	else if (std::string::npos != cs.find("PARROT"))
	{
		auto item = parrotMap.find(client);
		if (parrotMap.end() == item)
		{
			const auto ft = packet.GetFrameType();
			if (packet.IsStreamPacket())
			{
				if (not packet.IsLastPacket())
				{
					const CCallsign src(packet.GetCSrcAddress());
					if ((ft & 0x1du) == 0x5u)
					{
						// it is not the last packet and it is a stream packet and it is not enrypted
						std::cout << "Parrot stream from " << src << " on " << client->GetCallsign() << " with SID 0x" << std::hex << packet.GetStreamId() << std::dec << " at " << client->GetIp() << std::endl;
						parrotMap[client] = std::make_unique<CStreamParrot>(packet.GetCSrcAddress(), client, ft);
						parrotMap[client]->Add(packet);
					}
					else
					{
						std::cout << "Parrot stream from " << src << " on " << client->GetCallsign() << " was rejected because it was entrypted" << std::endl;
					}
				}
			}
			else
			{
				const CCallsign src(packet.GetCSrcAddress());
				std::cout << "Parrot Packet from " << src << " on " << client->GetCallsign() << " at " << client->GetIp() << std::endl;
				parrotMap[client] = std::make_unique<CPacketParrot>(packet.GetCSrcAddress(), client, ft);
				parrotMap[client]->Add(packet);
				parrotMap[client]->Play();
			}
		}
		else
		{
			if (EParrotState::record == item->second->GetState())
			{
				item->second->Add(packet);
				if (packet.IsLastPacket())
				{
					std::cout << "Parrot stream 0x" << std::hex << packet.GetStreamId() << std::dec << " closed, playing..." << std::endl;
					item->second->Play();
				}
			}
		}
		return false;
	}

	if ( client->IsListenOnly())
	{
		// std::cerr << "Client " << client->GetCallsign() << " is not allowed to stream! (ListenOnly)" << std::endl;
		return false;
	}

	if (packet.IsStreamPacket())
	{
		auto stream = GetStream(packet, client);
		if (stream)
		{
			// stream already open
			// skip packet, but tickle the stream
			stream->Tickle();
			return true;
		}
		else
		{
			// try to open the stream
			stream = OpenStream(packet, client);
			if ( nullptr == stream )
			{
				return false;
			}
		}
	}
	// update last heard
	CCallsign src(packet.GetCSrcAddress());
	auto cli = client->GetCallsign();
	g_Reflector.GetUsers()->Hearing(src, dst, cli, client->GetReflectorModule(), (packet.IsStreamPacket() ? EMode::sm : EMode::pm));
	g_Reflector.ReleaseUsers();
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////
// packet decoding helpers

bool CProtocol::IsValidConnect(const uint8_t *buf, CCallsign &cs, char &mod)
{
	if (0 == memcmp(buf, "CONN", 4))
	{
		cs.CodeIn(buf + 4);
		if (std::regex_match(cs.GetCS(), clientRegEx))
		{
			mod = buf[10];
			if (IsLetter(mod))
			{
				return true;
			}
			std::cout << "Bad CONN from '" << cs.GetCS() << "'." << std::endl;
			Dump("The requested module is not a letter:", buf, 11);
		}
		else
		{
			std::cout << "CONN packet rejected because '" << cs.GetCS() << "' didn't pass the regex!" << std::endl;
		}
	} else if (0 == memcmp(buf, "LSTN", 4)) {
		cs.CodeIn(buf + 4);
		if (std::regex_match(cs.GetCS(), lstnRegEx))
		{
			mod = buf[10];
			if (IsLetter(mod))
			{
				return true;
			}
			std::cout << "Bad LSTN from '" << cs.GetCS() << "'." << std::endl;
			Dump("The requested module is not a letter:", buf, 11);
		}
		else
		{
			std::cout << "LSTN packet rejected because '" << cs.GetCS() << "' didn't pass the regex!" << std::endl;
		}
	}
	return false;
}

bool CProtocol::IsValidDisconnect(const uint8_t *buf, CCallsign &cs)
{
	if (0 == memcmp(buf, "DISC", 4))
	{
		cs.CodeIn(buf + 4);
		auto call = cs.GetCS();
		if (std::regex_match(call, clientRegEx) || std::regex_match(call, peerRegEx) || std::regex_match(call, lstnRegEx))
		{
			return true;
		}
	}
	return false;
}

bool CProtocol::IsValidKeepAlive(const uint8_t *buf, CCallsign &cs)
{
	if ('P' == buf[0] && ('I' == buf[1] || 'O' == buf[1]) && 'N' ==  buf[2] && 'G' == buf[3])
	{
		cs.CodeIn(buf + 4);
		auto call = cs.GetCS();
		if (std::regex_match(call, clientRegEx) || std::regex_match(call, peerRegEx) || std::regex_match(call, lstnRegEx))
		{
			return true;
		}
	}
	return false;
}

bool CProtocol::IsValidPacket(CPacket &packet, size_t size, const char mod)
{
	auto buf = packet.GetCData();
	if (memcmp(buf, "M17", 3))
		return false;
	if ((' ' == char(buf[3])) and (0x1u == (0x1u & buf[19])) and ((54u == size) or (55u == size)))
	{
		packet.Initialize(54u, true);
	}
	else if (('P' == char(buf[3])) and ((37u < size) and (size <= MAX_PACKET_SIZE)) and (0x0u == (0x1u & buf[17])))
	{
		packet.Initialize(size, false);
	}
	else
	{
		return false;
	}
	// check validity of packet
	const CCallsign src(packet.GetCSrcAddress());
	if (std::regex_match(src.GetCS(), clientRegEx))
	{	// looks like a valid source
		if (packet.IsStreamPacket() and (0x18u & packet.GetFrameType()))
		{	// looks like this packet is encrypted
			if (g_CFG.IsEncyrptionAllowed(mod))
			{
				return true;
			}
			else
			{
				if (0u == packet.GetFrameNumber())	// we only log this once
					std::cout << "Blocking " << src.GetCS() << " to module " << mod << " because it is encrypted!" << std::endl;
			}
		}
		else
		{
			return true;
		}
	}
	else
	{
		std::cout << src.GetCS() << " Source C/S FAILED RegEx test" << std::endl;
	}
	return false;
}

bool CProtocol::IsValidInterlinkConnect(const uint8_t *buf, CCallsign &cs, char *mods)
{
	if (memcmp(buf, "CONN", 4))
		return false;

	cs.CodeIn(buf + 4);
	if (cs.GetCS(4).compare("M17-"))
	{
		std::cout << "Link request from '" << cs << "' denied" << std::endl;
		return false;
	}
	memcpy(mods, buf+10, 27);
	for (unsigned i=0; i<strlen(mods); i++)
	{
		if (! IsLetter(mods[i]))
		{
			std::cout << "Illegal module specified in '" << mods << "'" << std::endl;
			return false;
		}
	}
	return true;
}

bool CProtocol::IsValidInterlinkAcknowledge(const uint8_t *buf, CCallsign &cs, char *mods)
{
	SInterConnect *p = (SInterConnect *)buf;
	if (0 == memcmp(p->magic, "ACKN", 4))
	{
		cs.CodeIn(p->fromcs);
		memcpy(mods, p->mods, 27);
		return (0 == mods[26]);
	}
	return false;
}

bool CProtocol::IsValidNAcknowledge(const uint8_t *buf, CCallsign &cs)
{
	if (0 == memcmp(buf, "NACK", 4))
	{
		cs.CodeIn(buf + 4);
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////
// packet encoding helpers

void CProtocol::EncodeKeepAlivePacket(uint8_t *buf)
{
	memcpy(buf, "PING", 4);
	GetReflectorCallsign().CodeOut(buf + 4);
}

void CProtocol::EncodeInterlinkConnectPacket(SInterConnect &conn, const std::string &mods)
{
	memset(conn.magic, 0, sizeof(SInterConnect));
	memcpy(conn.magic, "CONN", 4);
	GetReflectorCallsign().CodeOut(conn.fromcs);
	memcpy(conn.mods, mods.c_str(), mods.size());
}

void CProtocol::EncodeConnectAckPacket(uint8_t *buf)
{
	memcpy(buf, "ACKN", 4);
}

void CProtocol::EncodeInterlinkAckPacket(SInterConnect &ackn, const char *mods)
{
	memset(ackn.magic, 0, sizeof(SInterConnect));
	memcpy(ackn.magic, "ACKN", 4);
	GetReflectorCallsign().CodeOut(ackn.fromcs);
	memcpy(ackn.mods, mods, strlen(mods));
}

void CProtocol::EncodeInterlinkNackPacket(uint8_t *buf)
{
	memcpy(buf, "NACK", 4);
	GetReflectorCallsign().CodeOut(buf+4);
}

void CProtocol::EncodeConnectNackPacket(uint8_t *buf)
{
	memcpy(buf, "NACK", 4);
}

void CProtocol::EncodeDisconnectPacket(uint8_t *buf, char mod)
{
	memcpy(buf, "DISC", 4);
	CCallsign cs(GetReflectorCallsign());
	if (mod)
		cs.SetModule(mod);
	cs.CodeOut(buf + 4);
}

void CProtocol::EncodeDisconnectedPacket(uint8_t *buf)
{
	memcpy(buf, "DISC", 4);
}

CPacketStream *CProtocol::OpenStream(CPacket &packet, SPClient client)
{
	// if it is a stream packet, check sid is not zero
	if ( packet.IsStreamPacket() and 0U == packet.GetStreamId() )
	{
		std::cerr << "Incoming stream has zero streamID" << std::endl;
		return nullptr;
	}

	if ( client->IsTransmitting() )
	{
		std::cerr << "Client " << client->GetCallsign() << " is already transmitting" << std::endl;
		return nullptr;
	}

	// get the module's packet stream queue
	const char module = client->GetReflectorModule();

	if (packet.IsStreamPacket())
	{
		// check if no stream with same streamid already open
		// to prevent loops
		for (auto &pit : m_streamMap)
		{
			if (pit.second->IsOpen() and (packet.GetStreamId() == pit.second->GetPacketStreamId()))
			{
				std::cerr << "Detected stream loop on module " << module << " for client " << client->GetCallsign() << " with sid " << packet.GetStreamId() << std::endl;
				return nullptr;
			}
		}
	}

	auto pit = m_streamMap.find(module);
	if (m_streamMap.end() == pit)
	{
		std::cerr << "Can't get stream from module '" << module << "'" << std::endl;
		return nullptr;
	}

	// is it available ?
	if ( pit->second->OpenPacketStream(packet, client) )
	{
		// stream open, mark client as master
		// so that it can't be deleted
		client->SetTX();

		// update last heard time
		client->Heard();

		// report
		const CCallsign src(packet.GetCSrcAddress());
		if (packet.IsStreamPacket())
			std::cout << "Opening stream on module " << module << " for client " << client->GetCallsign() << " with id 0x" << std::hex << packet.GetStreamId() << std::dec << " by user " << src << std::endl;
		else
			std::cout << "Opening packet on module " << module << " for client " << client->GetCallsign() << " by user " << src << std::endl;

		// and push the packet
		pit->second->Tickle();
		return pit->second.get();
	}
	return nullptr;
}

void CProtocol::CloseStream(char module)
{
	auto pit = m_streamMap.find(module);
	if (m_streamMap.end() == pit)
	{
		std::cerr << "Protocol::CLoseStream can find a packet stream for moudule '" << module << "'" << std::endl;
		return;
	}
	auto stream = pit->second.get();
	if (stream)
	{
		g_Reflector.GetClients();	// lock clients

		// get and check the master
		SPClient client = stream->GetOwnerClient();
		if ( client != nullptr )
		{
			// client no longer a master
			client->ClearTX();

			std::cout << "Closing stream on module " << module << std::endl;
		}

		// release clients
		g_Reflector.ReleaseClients();

		// and stop the queue
		stream->ClosePacketStream();
	}
}
