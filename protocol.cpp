//
//  cprotocol.cpp
//  mrefd
//
//  Created by Jean-Luc Deltombe (LX3JL) on 01/11/2015.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//  Copyright © 2022 Thomas A. Early, N7TAE
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

#include "main.h"
#include "protocol.h"
#include "clients.h"
#include "reflector.h"
#include "gatekeeper.h"
#include "configure.h"

////////////////////////////////////////////////////////////////////////////////////////
// constructor

CProtocol::CProtocol() : keep_running(true)
{
	peerRegEx = std::regex("^M17-([A-Z0-9]){3,3}(($)|( [A-Z]$))", std::regex::extended);
	clientRegEx = std::regex("^[0-9]?[A-Z]{1,2}[0-9]{1,2}[A-Z]{1,4}(($)|([ ]*[A-Z]?$)|([-/\\.][A-Z0-9]+$))", std::regex::extended);
}


////////////////////////////////////////////////////////////////////////////////////////
// destructor

CProtocol::~CProtocol()
{
	// kill threads
	Close();

	// empty queue
	m_Queue.Lock();
	while ( !m_Queue.empty() )
	{
		m_Queue.pop();
	}
	m_Queue.Unlock();
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
	uint8_t   buf[UDP_BUFFER_LENMAX];
	CIp       ip;
	CCallsign cs;
	char      mod;
	char      mods[27];
	std::unique_ptr<CPacket> pack;

	// any incoming packet ?
	auto len = (*this.*Receive)(buf, ip, 20);
	//if (len > 0) std::cout << "Received " << len << " bytes from " << ip << std::endl;
	switch (len) {
	case sizeof(SM17Frame):	// a packet from a client
	case sizeof(SRefM17Frame):	// a packet from a peer
		// check that the source and dest c/s is correct, including dest module
		if ( IsValidPacket(buf, (sizeof(SM17Frame) == len) ? false : true, pack) )
		{
			if (g_GateKeeper.MayTransmit(pack->GetSourceCallsign(), ip))
			{
				OnFirstPacketIn(pack, ip); // might open a new stream, if it's the first packet
				if (pack)                  // the packet might have been erased
				{                          // if it needed to open a new stream, but couldn't
					OnPacketIn(pack, ip);
				}
			}
			else if (pack->IsLastPacket())
			{
				std::cout << "Blocked voice stream from " << pack->GetSourceCallsign() << " at " << ip << std::endl;
			}
		}
		break;
	case sizeof(SInterConnect):
		if (IsValidInterlinkConnect(buf, cs, mods))
		{
			//std::cout << "CONN packet from " << cs <<  " at " << ip << " to module(s) " << mods << std::endl;

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
			//std::cout << "ACQN packet from " << cs << " at " << ip << " on module(s) " << mods << std::endl;

			// callsign authorized?
			if ( g_GateKeeper.MayLink(cs, ip, mods) )
			{
				// already connected ?
				CPeers *peers = g_Reflector.GetPeers();
				if ( nullptr == peers->FindPeer(cs, ip) )
				{
					// create the new peer
					// this also create one client per module
					std::shared_ptr<CPeer> peer = std::make_shared<CPeer>(cs, ip, mods);

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
				if ( g_CFG.IsValidModule(mod) )
				{
					// acknowledge a normal request from a repeater/hot-spot/mvoice
					EncodeConnectAckPacket(buf);
					Send(buf, 4, ip);

					// create the client and append
					g_Reflector.GetClients()->AddClient(std::make_shared<CClient>(cs, ip, mod));
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
				if ( peer )
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
					// this also remove all peer's clients from reflector client list
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
	m_Socket4.Close();
	m_Socket6.Close();
}

////////////////////////////////////////////////////////////////////////////////////////
// streams helpers

void CProtocol::OnPacketIn(std::unique_ptr<CPacket> &packet, const CIp &ip)
{
	// find the stream
	auto stream = GetStream(packet->GetStreamId(), ip);
	if ( stream )
	{
		auto islast = packet->IsLastPacket(); // we'll need this after the std::move()!

		// and push the packet
		stream->Lock();
		stream->Push(std::move(packet));
		stream->Unlock();

		if (islast)
			g_Reflector.CloseStream(stream);
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// stream handle helpers

std::shared_ptr<CPacketStream> CProtocol::GetStream(uint16_t uiStreamId, const CIp &Ip)
{
	for ( auto it=m_Streams.begin(); it!=m_Streams.end(); it++ )
	{
		if ( (*it)->GetPacketStreamId() == uiStreamId )
		{
			// if Ip not nullptr, also check if IP match
			if ( (*it)->GetOwnerIp() != nullptr )
			{
				if ( Ip == *((*it)->GetOwnerIp()) )
				{
					return *it;
				}
			}
		}
	}
	// done
	return nullptr;
}

void CProtocol::CheckStreamsTimeout(void)
{
	for ( auto it=m_Streams.begin(); it!=m_Streams.end(); )
	{
		// time out ?
		(*it)->Lock();
		if ( (*it)->IsExpired() )
		{
			// yes, close it
			(*it)->Unlock();
			g_Reflector.CloseStream(*it);
			// and remove it
			it = m_Streams.erase(it);
		}
		else
		{
			(*it++)->Unlock();
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
// dual stack senders

void CProtocol::Send(const char *buf, const CIp &Ip) const
{
	switch (Ip.GetFamily())
	{
	case AF_INET:
		m_Socket4.Send(buf, Ip);
		break;
	case AF_INET6:
		m_Socket6.Send(buf, Ip);
		break;
	default:
		std::cerr << "ERROR: wrong family: " << Ip.GetFamily() << std::endl;
		break;
	}
}

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

void CProtocol::Send(const char *buf, const CIp &Ip, uint16_t port) const
{
	switch (Ip.GetFamily())
	{
	case AF_INET:
		m_Socket4.Send(buf, Ip, port);
		break;
	case AF_INET6:
		m_Socket6.Send(buf, Ip, port);
		break;
	default:
		std::cerr << "ERROR: wrong family: " << Ip.GetFamily() << " on port " << port << std::endl;
		break;
	}
}

void CProtocol::Send(const uint8_t *buf, size_t size, const CIp &Ip, uint16_t port) const
{
	switch (Ip.GetFamily())
	{
	case AF_INET:
		m_Socket4.Send(buf, size, Ip, port);
		break;
	case AF_INET6:
		m_Socket6.Send(buf, size, Ip, port);
		break;
	default:
		std::cerr << "ERROR: wrong family: " << Ip.GetFamily() << " on port " << port << std::endl;
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// queue helper

void CProtocol::HandleQueue(void)
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
			if ( !client->IsTransmitting() && (client->GetReflectorModule() == packet->GetDestModule()) )
			{
				auto cs = client->GetCallsign();

				if (cs.GetCS(4).compare("M17-"))
				{
					// the client is not a reflector
					cs.CodeOut(packet->GetFrame().frame.lich.addr_dst);
					packet->SetCRC(crc.CalcCRC(packet->GetFrame().frame.magic, sizeof(SM17Frame) - 2));
					Send(packet->GetFrame().frame.magic, sizeof(SM17Frame), client->GetIp());
				}
				else if (! packet->GetRelay())
				{
					// the client is a reflector and the packet hasn't yet been relayed
					cs.SetModule(client->GetReflectorModule());
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

void CProtocol::HandleKeepalives(void)
{
	uint8_t keepalive[10];
	EncodeKeepAlivePacket(keepalive);

	// iterate on clients
	CClients *clients = g_Reflector.GetClients();
	auto it = clients->begin();
	std::shared_ptr<CClient> client;
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
		else if ( !client->IsAlive() )
		{
			CPeers *peers = g_Reflector.GetPeers();
			auto peer = peers->FindPeer(client->GetCallsign(), client->GetIp());
			if ( peer && (peer->GetReflectorModules()[0] == client->GetReflectorModule()) )
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
				std::cout << "Client " << client->GetCallsign() << " keepalive timeout" << std::endl;
				clients->RemoveClient(client);
			}
			g_Reflector.ReleasePeers();
		}

	}
	g_Reflector.ReleaseClients();

	// iterate on peers
	CPeers *peers = g_Reflector.GetPeers();
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

void CProtocol::HandlePeerLinks(void)
{
	static bool publish = true;
	uint8_t buf[10];
	// get the list of peers
	CPeerMap *peermap = g_GateKeeper.GetPeerMap();
	CPeers *peers = g_Reflector.GetPeers();

	// check if all our connected peers are still listed by gatekeeper
	// if not, disconnect
	auto pit = peers->begin();
	std::shared_ptr<CPeer>peer = nullptr;
	while ( (peer = peers->FindNextPeer(pit)) != nullptr )
	{
		if ( nullptr == peermap->FindMapItem(peer->GetCallsign().GetCS()) )
		{
			publish = true;
			// send disconnect packet
			EncodeDisconnectPacket(buf, 0);
			Send(buf, 10, peer->GetIp());
			std::cout << "Sent disconnect packet to M17 peer " << peer->GetCallsign() << " at " << peer->GetIp() << std::endl;
			// remove client
			peers->RemovePeer(peer);
		}
	}

	// check if all ours peers listed by gatekeeper are connected
	// if not, connect or reconnect
	SInterConnect connect;
	for ( auto it=peermap->begin(); it!=peermap->end(); it++ )
	{
		if ( nullptr == peers->FindPeer((*it).second.GetCallsign()) )
		{
			publish = true;
			// send connect packet to re-initiate peer link
			EncodeInterlinkConnectPacket(connect, (*it).second.GetModules());
			Send(connect.magic, sizeof(SInterConnect), (*it).second.GetIp());
			std::cout << "Sent connect packet to M17 peer " << (*it).second.GetCallsign() << " @ " << (*it).second.GetIp() << " for module(s) " << (*it).second.GetModules() << std::endl;
		}
	}

	g_Reflector.ReleasePeers();
	g_GateKeeper.ReleasePeerMap();

	if (publish)
	{
		g_GateKeeper.PutDHTInfo();
		publish = false;
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// streams helpers

void CProtocol::OnFirstPacketIn(std::unique_ptr<CPacket> &packet, const CIp &ip)
{
	// find the stream
	auto stream = GetStream(packet->GetStreamId(), ip);
	if ( stream )
	{
		// stream already open
		// skip packet, but tickle the stream
		stream->Tickle();
	}
	else
	{
		// find this client
		std::shared_ptr<CClient> client = g_Reflector.GetClients()->FindClient(ip);
		if ( client )
		{
			// save the source and destination module for Hearing().
			// We're going to lose packet after the OpenStream() call.
			auto s = packet->GetSourceCallsign();
			auto d = packet->GetDestCallsign().GetModule();
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
				auto from = client->GetCallsign();
				if (0 == from.GetCS(4).compare("M17-"))
					from.SetModule(d);
				auto ref = GetReflectorCallsign();
				ref.SetModule(d);
				g_Reflector.GetUsers()->Hearing(s, from, ref);
				g_Reflector.ReleaseUsers();
			}

		}
		// release
		g_Reflector.ReleaseClients();
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// packet decoding helpers

bool CProtocol::IsValidConnect(const uint8_t *buf, CCallsign &cs, char *mod)
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

bool CProtocol::IsValidDisconnect(const uint8_t *buf, CCallsign &cs)
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

bool CProtocol::IsValidKeepAlive(const uint8_t *buf, CCallsign &cs)
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

bool CProtocol::IsValidPacket(const uint8_t *buf, bool is_internal, std::unique_ptr<CPacket> &packet)
{
	if (0 == memcmp(buf, "M17 ", 4))	// we tested the size before we got here
	{
		// create packet
		packet = std::unique_ptr<CPacket>(new CPacket(buf, is_internal));
		// check validity of packet
		auto dest = packet->GetDestCallsign();
		if (g_CFG.IsValidModule(dest.GetModule()) && dest.HasSameCallsign(GetReflectorCallsign()))
		{
			if (std::regex_match(packet->GetSourceCallsign().GetCS(), clientRegEx))
			{	// looks like a valid source
				return true;
			}
			else
			{
				std::cout << packet->GetSourceCallsign().GetCS() << " Source C/S FAILED RegEx test" << std::endl;
			}
		}
		else
		{
			std::cout << "Destination " << dest << " is invalid" << std::endl;
		}
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

bool CProtocol::IsVaildInterlinkAcknowledge(const uint8_t *buf, CCallsign &cs, char *mods)
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
