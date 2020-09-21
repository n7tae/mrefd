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
// operation

bool CM17Protocol::Initialize(int ptype, const uint16_t port, const bool has_ipv4, const bool has_ipv6)
{
	// base class
	if (! CProtocol::Initialize(ptype, port, has_ipv4, has_ipv6))
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
	std::unique_ptr<CPacket> pack;

	// any incoming packet ?
#if DSTAR_IPV6==true
#if DSTAR_IPV4==true
	auto len = ReceiveDS(buf, ip, 20);
#else
	auto len = Receive6(buf, ip, 20);
#endif
#else
	auto len = Receive4(buf, ip, 20);
#endif
	switch (len) {
	case sizeof(AM17Frame):
		if ( IsValidPacket(buf, pack) )
		{
			if (pack->GetDestCallsign().HasSameCallsign(g_Reflector.GetCallsign()))
			{	// only if the packet has the destination set properly!
				OnFirstPacketIn(pack, ip); // might open a new stream, if it's the first packet
				if (pack)                  // the packet might have been erased
				{                          // if it needed to open a new stream, but couldn't
					OnPacketIn(pack, ip);
				}
			}
		}
		break;
	case 11:
		if ( IsValidConnect(buf, cs, &mod) )
		{
			std::cout << "Connect packet for module " << mod << " from " << cs << " at " << ip << std::endl;

			// callsign authorized?
			if ( g_GateKeeper.MayLink(cs, ip, PROTOCOL_M17) )
			{
				// valid module ?
				if ( g_Reflector.IsValidModule(mod) )
				{
					// is this an ack for a link request?
					CPeerCallsignList *list = g_GateKeeper.GetPeerList();
					CCallsignListItem *item = list->FindListItem(cs);
					if ( item != nullptr && cs.GetModule() == item->GetModules()[1] && mod == item->GetModules()[0] )
					{
						std::cout << "ACK packet for module " << mod << " from " << cs << " at " << ip << std::endl;

						// already connected ?
						CPeers *peers = g_Reflector.GetPeers();
						if ( nullptr == peers->FindPeer(cs, ip, PROTOCOL_M17) )
						{
							// create the new peer
							// this also create one client per module
							// append the peer to reflector peer list
							// this also add all new clients to reflector client list
							peers->AddPeer(std::make_shared<CM17Peer>(cs, ip, std::string(1, mod).c_str()));
						}
						g_Reflector.ReleasePeers();
					}
					else
					{
						// acknowledge the request
						EncodeConnectAckPacket(buf);
						Send(buf, 4, ip);

						// create the client and append
						g_Reflector.GetClients()->AddClient(std::make_shared<CM17Client>(cs, ip, mod));
						g_Reflector.ReleaseClients();
					}
					g_GateKeeper.ReleasePeerList();
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
			// find all clients with that callsign & ip and keep them alive
			CClients *clients = g_Reflector.GetClients();
			auto it = clients->begin();
			std::shared_ptr<CClient>client = nullptr;
			while (nullptr != (client = clients->FindNextClient(cs, ip, PROTOCOL_M17, it)))
			{
				client->Alive();
			}
			g_Reflector.ReleaseClients();
		}
		else if ( IsValidDisconnect(buf, cs) )
		{
			std::cout << "Disconnect packet from " << cs << " at " << ip << std::endl;

			// find client & remove it
			CClients *clients = g_Reflector.GetClients();
			std::shared_ptr<CClient>client = clients->FindClient(ip, PROTOCOL_M17);
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
		while (nullptr != (client = clients->FindNextClient(PROTOCOL_M17, it)))
		{
			// is this client busy ?
			if ( !client->IsAMaster() && (client->GetReflectorModule() == packet->GetDestModule()) )
			{
				client->GetCallsign().EncodeCallsign(packet->GetFrame().lich.addr_dst);
				packet->SetCRC(crc.CalcCRC(packet->GetFrame().magic, sizeof(AM17Frame) - 2));
				Send(packet->GetFrame().magic, sizeof(AM17Frame), client->GetIp());
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
	// DExtra protocol sends and monitors keepalives packets
	// event if the client is currently streaming
	// so, send keepalives to all
	uint8_t keepalive[10];
	EncodeKeepAlivePacket(keepalive);

	// iterate on clients
	CClients *clients = g_Reflector.GetClients();
	auto it = clients->begin();
	std::shared_ptr<CClient>client = nullptr;
	while ( (client = clients->FindNextClient(PROTOCOL_M17, it)) != nullptr )
	{
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
			std::shared_ptr<CPeer>peer = peers->FindPeer(client->GetCallsign(), client->GetIp(), PROTOCOL_M17);
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
				std::cout << "DExtra client " << client->GetCallsign() << " keepalive timeout" << std::endl;
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
	while ( (peer = peers->FindNextPeer(PROTOCOL_M17, pit)) != nullptr )
	{
		// keepalives are sent between clients

		// some client busy or still with us ?
		if ( !peer->IsAMaster() && !peer->IsAlive() )
		{
			// no, disconnect all clients
			uint8_t disconnect[10];
			EncodeDisconnectPacket(disconnect, peer->GetReflectorModules()[0]);
			CClients *clients = g_Reflector.GetClients();
			for ( auto cit=peer->cbegin(); cit!=peer->cend(); cit++ )
			{
				Send(disconnect, 10, (*cit)->GetIp());
			}
			g_Reflector.ReleaseClients();

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
	while ( (peer = peers->FindNextPeer(PROTOCOL_M17, pit)) != nullptr )
	{
		if ( list->FindListItem(peer->GetCallsign()) == nullptr )
		{
			// send disconnect packet
			EncodeDisconnectPacket(buf, peer->GetReflectorModules()[0]);
			Send(buf, 10, peer->GetIp());
			std::cout << "Sending disconnect packet to XRF peer " << peer->GetCallsign() << " at " << peer->GetIp() << std::endl;
			// remove client
			peers->RemovePeer(peer);
		}
	}

	// check if all ours peers listed by gatekeeper are connected
	// if not, connect or reconnect
	uint8_t connect[11];
	for ( auto it=list->begin(); it!=list->end(); it++ )
	{
		if ( !(*it).GetCallsign().HasSameCallsignWithWildcard(CCallsign("XRF*")) )
			continue;
		if ( strlen((*it).GetModules()) != 2 )
			continue;
		if ( nullptr == peers->FindPeer((*it).GetCallsign(), PROTOCOL_M17) )
		{
			// resolve again peer's IP in case it's a dynamic IP
			(*it).ResolveIp();
			// send connect packet to re-initiate peer link
			EncodeConnectPacket(connect, (*it).GetModules());
			Send(connect, 11, (*it).GetIp());
			std::cout << "Sending connect packet to XRF peer " << (*it).GetCallsign() << " @ " << (*it).GetIp() << " for module " << (*it).GetModules()[1] << " (module " << (*it).GetModules()[0] << ")" << std::endl;
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
		std::shared_ptr<CClient>client = g_Reflector.GetClients()->FindClient(ip, PROTOCOL_M17);
		if ( client )
		{
			// try to open the stream
			stream = g_Reflector.OpenStream(packet, client);
			if ( nullptr == stream )
			{
				packet.release();	// couldn't open the stream, so destroy the packet
			}
			else
			{
				// keep the handle
				m_Streams.push_back(stream);

				// update last heard
				g_Reflector.GetUsers()->Hearing(packet->GetSourceCallsign(), packet->GetDestCallsign());
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
		cs.DecodeCallsign(buf + 10);
		if (cs.IsValid())
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
		cs.DecodeCallsign(buf+4);
		if (cs.IsValid())
		{
			return true;
		}
	}
	return false;
}

bool CM17Protocol::IsValidKeepAlive(const uint8_t *buf, CCallsign &cs)
{
	if (0 == memcmp(buf, "PING", 4))
	{
		cs.DecodeCallsign(buf+4);
		if (cs.IsValid())
		{
			return true;
		}
	}
	return false;
}

bool CM17Protocol::IsValidPacket(const uint8_t *buf, std::unique_ptr<CPacket> &packet)
{
	if (0 == memcmp(buf, "M17 ", 4))	// we tested the size before we got here
	{
		// create packet
		packet = std::unique_ptr<CPacket>(new CPacket(buf));
		// check validity of packet
		if ( packet->GetSourceCallsign().IsValid() )
		{	// looks like a valid source
			return true;
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////
// packet encoding helpers

void CM17Protocol::EncodeKeepAlivePacket(uint8_t *buf)
{
	memcpy(buf, "PING", 4);
	CCallsign cs(GetReflectorCallsign());
	cs.EncodeCallsign(buf + 4);
}

void CM17Protocol::EncodeConnectPacket(uint8_t *buf, const char *Modules)
{
	memcpy(buf, "CONN", 4);
	CCallsign cs(GetReflectorCallsign());
	cs.SetModule(Modules[0]);
	cs.EncodeCallsign(buf + 4);
	buf[10] = Modules[1];
}

void CM17Protocol::EncodeConnectAckPacket(uint8_t *buf)
{
	memcpy(buf, "ACKN", 4);
}

void CM17Protocol::EncodeConnectNackPacket(uint8_t *buf)
{
	memcpy(buf, "NACK", 4);
}

void CM17Protocol::EncodeDisconnectPacket(uint8_t *buf, char mod)
{
	memcpy(buf, "DISC", 4);
	CCallsign cs(GetReflectorCallsign());
	cs.SetModule(mod);
	cs.EncodeCallsign(buf + 4);
}

void CM17Protocol::EncodeDisconnectedPacket(uint8_t *buf)
{
	memcpy(buf, "DISC", 4);
}
