//
//  Created by Jean-Luc Deltombe (LX3JL) on 01/11/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2020 Thomas A. Early, N7TAE
//
// ----------------------------------------------------------------------------
//    This file is part of mrefd.
//
//    mrefd is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    mrefd is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <string.h>

#include "main.h"
#include "m17peer.h"
#include "m17client.h"
#include "m17protocol.h"
#include "reflector.h"
#include "gatekeeper.h"


////////////////////////////////////////////////////////////////////////////////////////
// constructor

CM17Protocol::CM17Protocol()
{
	peerRegEx = std::regex("^M17-([A-Z0-9]){3,3}[ ][A-Z]$", std::regex::extended);
	clientRegEx = std::regex("^[0-9]?[A-Z]{1,2}[0-9][A-Z]{1,4}(()|([ ]*[A-Z]?)|([-/\\.][A-Z0-9]+))$", std::regex::extended);
}

////////////////////////////////////////////////////////////////////////////////////////
// operation

bool CM17Protocol::Initialize(const uint16_t port, const bool has_ipv4, const bool has_ipv6)
{
	// base class
	if (! CProtocol::Initialize(port, has_ipv4, has_ipv6))
		return false;

	// update time
	m_LastKeepaliveTime.Now();
	m_LastPeersLinkTime.Now();

	// done
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////
// task

void CM17Protocol::Task(void)
{
	uint8_t   buf[UDP_BUFFER_LENMAX];
	CIp       ip;
	CCallsign cs;
	char      mod;
	char      mods[27];
	std::unique_ptr<CPacket> pack;

	// any incoming packet ?
#ifdef LISTEN_IPV6
#ifdef LISTEN_IPV4
	auto len = ReceiveDS(buf, ip, 20);
#else
	auto len = Receive6(buf, ip, 20);
#endif
#else
	auto len = Receive4(buf, ip, 20);
#endif
	//if (len > 0) std::cout << "Received " << len << " bytes from " << ip << std::endl;
	switch (len) {
	case sizeof(SM17Frame):	// a packet from a client
	case sizeof(SRefM17Frame):	// a packet from a peer
		if ( IsValidPacket(buf, (sizeof(SM17Frame) == len) ? false : true, pack) )
		{
			if (pack->GetDestCallsign().HasSameCallsign(g_Reflector.GetCallsign()))
			{	// only if the packet has the destination set properly!
				OnFirstPacketIn(pack, ip); // might open a new stream, if it's the first packet
				if (pack)                  // the packet might have been erased
				{                          // if it needed to open a new stream, but couldn't
					OnPacketIn(pack, ip);
				}
			}
			else
			{
				CCallsign dest(buf+6);
				std::cout << "Packet with wrong destination:" << dest.GetCS() << std::endl;
			}
		}
		break;
	case sizeof(SInterConnect):
		if (IsValidInterlinkConnect(buf, cs, mods))
		{
			std::cout << "CONN packet from " << cs <<  " at " << ip << " to module(s) " << mods << std::endl;

			// callsign authorized?
			if ( g_GateKeeper.MayLink(cs, ip, mods) )
			{
				SInterConnect ackn;
				// acknowledge the request
				EncodeInterlinkAckPacket(ackn, mods);
				Send(ackn.magic, sizeof(SInterConnect), ip);
			}
			else
			{
				// deny the request
				EncodeInterlinkNackPacket(buf);
				Send(buf, 10, ip);
			}

		}
		else if (IsVaildInterlinkAcknowledge(buf, cs, mods))
		{
			std::cout << "ACQN packet from " << cs << " at " << ip << " on module(s) " << mods << std::endl;

			// callsign authorized?
			if ( g_GateKeeper.MayLink(cs, ip) )
			{
				// already connected ?
				CPeers *peers = g_Reflector.GetPeers();
				if ( nullptr == peers->FindPeer(cs, ip) )
				{
					// create the new peer
					// this also create one client per module
					auto peer = std::make_shared<CM17Peer>(cs, ip, mods);

					// append the peer to reflector peer list
					// this also add all new clients to reflector client list
					peers->AddPeer(peer);
				}
				g_Reflector.ReleasePeers();
			}
		}
		break;
	case 11:
		if ( IsValidConnect(buf, cs, &mod) )
		{
			std::cout << "Connect packet for module " << mod << " from " << cs << " at " << ip << std::endl;

			// callsign authorized?
			if ( g_GateKeeper.MayLink(cs, ip) )
			{
				// valid module ?
				if ( g_Reflector.IsValidModule(mod) )
				{
					// acknowledge a normal request from a repeater/hot-spot/mvoice
					EncodeConnectAckPacket(buf);
					Send(buf, 4, ip);

					// create the client and append
					g_Reflector.GetClients()->AddClient(std::make_shared<CM17Client>(cs, ip, mod));
					g_Reflector.ReleaseClients();
				}
				else
				{
					std::cout << "Node " << cs << " connect attempt on non-existing module '" << mod << "'" << std::endl;

					// deny the request
					EncodeConnectNackPacket(buf);
					Send(buf, 4, ip);
				}
			}
			else
			{
				// deny the request
				EncodeConnectNackPacket(buf);
				Send(buf, 4, ip);
			}
		}
		break;
	case 10:
		if ( IsValidKeepAlive(buf, cs) )
		{
			if (cs.GetCS(4).compare("M17-")) {
				// find all clients with that callsign & ip and keep them alive
				CClients *clients = g_Reflector.GetClients();
				auto it = clients->begin();
				std::shared_ptr<CClient>client = nullptr;
				while (nullptr != (client = clients->FindNextClient(cs, ip, it)))
				{
					client->Alive();
				}
				g_Reflector.ReleaseClients();
			}
			else
			{
				// find peer
				CPeers *peers = g_Reflector.GetPeers();
				std::shared_ptr<CPeer>peer = peers->FindPeer(ip);
				if ( peer != nullptr )
				{
					// keep it alive
					peer->Alive();
				}
				g_Reflector.ReleasePeers();
			}
		}
		else if ( IsValidDisconnect(buf, cs) )
		{
			std::cout << "Disconnect packet from " << cs << " at " << ip << std::endl;
			if (cs.GetCS(4).compare("M17-")) {
				// find the regular client & remove it
				CClients *clients = g_Reflector.GetClients();
				std::shared_ptr<CClient>client = clients->FindClient(ip);
				if ( client != nullptr )
				{
					// ack disconnect packet
					EncodeDisconnectedPacket(buf);
					Send(buf, 4, ip);
					// and remove it
					clients->RemoveClient(client);
				}
				g_Reflector.ReleaseClients();
			}
			else
			{
				// find the peer and remove it
				CPeers *peers = g_Reflector.GetPeers();
				std::shared_ptr<CPeer>peer = peers->FindPeer(ip);
				if ( peer )
				{
					// remove it from reflector peer list
					// this also remove all concerned clients from reflector client list
					// and delete them
					peers->RemovePeer(peer);
				}
				g_Reflector.ReleasePeers();
			}
		}
		else if ( IsValidNAcknowledge(buf, cs))
		{
			std::cout << "NACK packet received from " << cs << " at " << ip << std::endl;
		}
		break;
	default:
		break;
	}

	// handle end of streaming timeout
	CheckStreamsTimeout();

	// handle queue from reflector
	HandleQueue();

	// keep alive
	if ( m_LastKeepaliveTime.DurationSinceNow() > M17_KEEPALIVE_PERIOD )
	{
		// handle keep alives
		HandleKeepalives();

		// update time
		m_LastKeepaliveTime.Now();
	}

	// peer connections
	if ( m_LastPeersLinkTime.DurationSinceNow() > M17_RECONNECT_PERIOD )
	{
		// handle remote peers connections
		HandlePeerLinks();

		// update time
		m_LastPeersLinkTime.Now();
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// queue helper

void CM17Protocol::HandleQueue(void)
{
	m_Queue.Lock();
	while ( !m_Queue.empty() )
	{
		// get the packet
		auto packet = m_Queue.front();
		m_Queue.pop();

		// push it to all our clients linked to the module and who is not streaming in
		CClients *clients = g_Reflector.GetClients();
		auto it = clients->begin();
		std::shared_ptr<CClient>client = nullptr;
		while (nullptr != (client = clients->FindNextClient(it)))
		{
			// is this client busy ?
			if ( !client->IsAMaster() && (client->GetReflectorModule() == packet->GetDestModule()) )
			{
				auto cs = client->GetCallsign();
				auto comp = cs.GetCS(4).compare("M17-");

				if (comp) // the client is not a reflector
				{
					cs.CodeOut(packet->GetFrame().frame.lich.addr_dst);
					packet->SetCRC(crc.CalcCRC(packet->GetFrame().frame.magic, sizeof(SM17Frame) - 2));
					Send(packet->GetFrame().frame.magic, sizeof(SM17Frame), client->GetIp());
				}
				else if (! packet->GetRelay())// the client is a reflector and the packet hasn't yet been relayed
				{
					cs.CodeOut(packet->GetFrame().frame.lich.addr_dst);	      // set the destination
					packet->SetCRC(crc.CalcCRC(packet->GetFrame().frame.magic, sizeof(SM17Frame) - 2)); // recalculate the crc
					packet->SetRelay(true);  // make sure the destination reflector doesn't send it to other reflectors
					Send(packet->GetFrame().frame.magic, sizeof(SRefM17Frame), client->GetIp());
					packet->SetRelay(false); // reset for the next client;
				}
			}
		}
		g_Reflector.ReleaseClients();
	}
	m_Queue.Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////
// keepalive helpers

void CM17Protocol::HandleKeepalives(void)
{
	uint8_t keepalive[10];
	EncodeKeepAlivePacket(keepalive);

	// iterate on clients
	CClients *clients = g_Reflector.GetClients();
	auto it = clients->begin();
	std::shared_ptr<CClient>client = nullptr;
	while ( nullptr != (client = clients->FindNextClient(it)) )
	{
		// don't ping reflector modules, we'll do each interlinked refectors below
		if (0 == client->GetCallsign().GetCS(4).compare("M17-"))
			continue;

		// send keepalive
		Send(keepalive, 10, client->GetIp());

		// client busy ?
		if ( client->IsAMaster() )
		{
			// yes, just tickle it
			client->Alive();
		}
		// otherwise check if still with us
		else if ( !client->IsAlive() )
		{
			CPeers *peers = g_Reflector.GetPeers();
			std::shared_ptr<CPeer>peer = peers->FindPeer(client->GetCallsign(), client->GetIp());
			if ( (peer != nullptr) && (peer->GetReflectorModules()[0] == client->GetReflectorModule()) )
			{
				// no, but this is a peer client, so it will be handled below
			}
			else
			{
				// no, disconnect
				uint8_t disconnect[10];
				EncodeDisconnectPacket(disconnect, client->GetReflectorModule());
				Send(disconnect, 10, client->GetIp());

				// remove it
				std::cout << "M17 client " << client->GetCallsign() << " keepalive timeout" << std::endl;
				clients->RemoveClient(client);
			}
			g_Reflector.ReleasePeers();
		}

	}
	g_Reflector.ReleaseClients();

	// iterate on peers
	CPeers *peers = g_Reflector.GetPeers();
	auto pit = peers->begin();
	std::shared_ptr<CPeer>peer = nullptr;
	while ( nullptr != (peer = peers->FindNextPeer(pit)) )
	{
		// send keepalive
		Send(keepalive, 10, peer->GetIp());

		// client busy ?
		if ( peer->IsAMaster() )
		{
			// yes, just tickle it
			peer->Alive();
		}
		// otherwise check if still with us
		else if ( !peer->IsAlive() )
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

void CM17Protocol::HandlePeerLinks(void)
{
	uint8_t buf[10];
	// get the list of peers
	CPeerCallsignList *list = g_GateKeeper.GetPeerList();
	CPeers *peers = g_Reflector.GetPeers();

	// check if all our connected peers are still listed by gatekeeper
	// if not, disconnect
	auto pit = peers->begin();
	std::shared_ptr<CPeer>peer = nullptr;
	while ( (peer = peers->FindNextPeer(pit)) != nullptr )
	{
		if ( list->FindListItem(peer->GetCallsign()) == nullptr )
		{
			// send disconnect packet
			EncodeDisconnectPacket(buf, 0);
			Send(buf, 10, peer->GetIp());
			std::cout << "Sending disconnect packet to M17 peer " << peer->GetCallsign() << " at " << peer->GetIp() << std::endl;
			// remove client
			peers->RemovePeer(peer);
		}
	}

	// check if all ours peers listed by gatekeeper are connected
	// if not, connect or reconnect
	SInterConnect connect;
	for ( auto it=list->begin(); it!=list->end(); it++ )
	{
		if ( (*it).GetCallsign().HasSameCallsignWithWildcard(CCallsign("M17-*")) )
		{
			if ( nullptr == peers->FindPeer((*it).GetCallsign()) )
			{
				// send connect packet to re-initiate peer link
				EncodeInterlinkConnectPacket(connect, (*it).GetModules());
				Send(connect.magic, sizeof(SInterConnect), (*it).GetIp());
				std::cout << "Sent connect packet to M17 peer " << (*it).GetCallsign() << " @ " << (*it).GetIp() << " for module(s) " << (*it).GetModules() << std::endl;
			}
		}
	}

	// done
	g_Reflector.ReleasePeers();
	g_GateKeeper.ReleasePeerList();
}

////////////////////////////////////////////////////////////////////////////////////////
// streams helpers

void CM17Protocol::OnFirstPacketIn(std::unique_ptr<CPacket> &packet, const CIp &ip)
{
	// find the stream
	CPacketStream *stream = GetStream(packet->GetStreamId(), ip);
	if ( stream )
	{
		// stream already open
		// skip packet, but tickle the stream
		stream->Tickle();
	}
	else
	{
		// find this client
		std::shared_ptr<CClient>client = g_Reflector.GetClients()->FindClient(ip);
		if ( client )
		{
			// save the source and destination for Hearing().
			// We're going to lose packet on OpenStream();
			auto s = packet->GetSourceCallsign();
			auto d = packet->GetDestCallsign();
			// try to open the stream
			stream = g_Reflector.OpenStream(packet, client);
			if ( nullptr == stream )
			{
				std::cerr << "Cant open the stream for " << client->GetCallsign() << std::endl;
				packet.release();	// couldn't open the stream, so destroy the packet
			}
			else
			{
				// keep the handle
				m_Streams.push_back(stream);

				// update last heard
				g_Reflector.GetUsers()->Hearing(s, d);
				g_Reflector.ReleaseUsers();
			}

		}
		// release
		g_Reflector.ReleaseClients();
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// packet decoding helpers

bool CM17Protocol::IsValidConnect(const uint8_t *buf, CCallsign &cs, char *mod)
{
	if (0 == memcmp(buf, "CONN", 4))
	{
		cs.CodeIn(buf + 4);
		if (std::regex_match(cs.GetCS(), clientRegEx))
		{
			*mod = buf[10];
			if (IsLetter(*mod))
				return true;
		}
	}
	return false;
}

bool CM17Protocol::IsValidDisconnect(const uint8_t *buf, CCallsign &cs)
{
	if (0 == memcmp(buf, "DISC", 4))
	{
		cs.CodeIn(buf + 4);
		auto call = cs.GetCS();
		if (std::regex_match(call, clientRegEx) || std::regex_match(call, peerRegEx))
		{
			return true;
		}
	}
	return false;
}

bool CM17Protocol::IsValidKeepAlive(const uint8_t *buf, CCallsign &cs)
{
	if ('P' == buf[0] && ('I' == buf[1] || 'O' == buf[1]) && 'N' ==  buf[2] && 'G' == buf[3])
	{
		cs.CodeIn(buf + 4);
		auto call = cs.GetCS();
		if (std::regex_match(call, clientRegEx) || std::regex_match(call, peerRegEx))
		{
			return true;
		}
	}
	return false;
}

bool CM17Protocol::IsValidPacket(const uint8_t *buf, bool is_internal, std::unique_ptr<CPacket> &packet)
{
	if (0 == memcmp(buf, "M17 ", 4))	// we tested the size before we got here
	{
		// create packet
		packet = std::unique_ptr<CPacket>(new CPacket(buf, is_internal));
		// check validity of packet
		auto dest = packet->GetDestCallsign().GetCS();
		if (std::regex_match(dest, clientRegEx) || std::regex_match(dest, peerRegEx))
		{
			if (std::regex_match(packet->GetSourceCallsign().GetCS(), clientRegEx))
			{	// looks like a valid source
				return true;
			}
		}
	}
	return false;
}

bool CM17Protocol::IsValidInterlinkConnect(const uint8_t *buf, CCallsign &cs, char *mods)
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

bool CM17Protocol::IsVaildInterlinkAcknowledge(const uint8_t *buf, CCallsign &cs, char *mods)
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

bool CM17Protocol::IsValidNAcknowledge(const uint8_t *buf, CCallsign &cs)
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

void CM17Protocol::EncodeKeepAlivePacket(uint8_t *buf)
{
	memcpy(buf, "PING", 4);
	CCallsign cs(GetReflectorCallsign());
	cs.CodeOut(buf + 4);
}

void CM17Protocol::EncodeInterlinkConnectPacket(SInterConnect &conn, const std::string &mods)
{
	memset(conn.magic, 0, sizeof(SInterConnect));
	memcpy(conn.magic, "CONN", 4);
	GetReflectorCallsign().CodeOut(conn.fromcs);
	memcpy(conn.mods, mods.c_str(), mods.size());
}

void CM17Protocol::EncodeConnectAckPacket(uint8_t *buf)
{
	memcpy(buf, "ACKN", 4);
}

void CM17Protocol::EncodeInterlinkAckPacket(SInterConnect &ackn, const char *mods)
{
	memset(ackn.magic, 0, sizeof(SInterConnect));
	memcpy(ackn.magic, "ACKN", 4);
	CCallsign cs(GetReflectorCallsign());
	cs.CodeOut(ackn.fromcs);
	memcpy(ackn.mods, mods, strlen(mods));
}

void CM17Protocol::EncodeInterlinkNackPacket(uint8_t *buf)
{
	memcpy(buf, "NACK", 4);
	CCallsign cs(GetReflectorCallsign());
	cs.CodeOut(buf+4);
}

void CM17Protocol::EncodeConnectNackPacket(uint8_t *buf)
{
	memcpy(buf, "NACK", 4);
}

void CM17Protocol::EncodeDisconnectPacket(uint8_t *buf, char mod)
{
	memcpy(buf, "DISC", 4);
	CCallsign cs(GetReflectorCallsign());
	if (mod)
		cs.SetModule(mod);
	cs.CodeOut(buf + 4);
}

void CM17Protocol::EncodeDisconnectedPacket(uint8_t *buf)
{
	memcpy(buf, "DISC", 4);
}
