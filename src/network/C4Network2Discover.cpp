/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */
#include "C4Include.h"
#include "C4Network2Discover.h"

// *** C4Network2IODiscover

struct C4Network2IODiscoverReply
{
	char c;
	uint16_t Port;
};

void C4Network2IODiscover::OnPacket(const class C4NetIOPacket &rPacket, C4NetIO *pNetIO)
{
	// discovery?
	if (fEnabled && rPacket.getSize() == 1 && rPacket.getStatus() == 3)
		Announce();
}

bool C4Network2IODiscover::Init(uint16_t iPort)
{
	// Reuse address
	C4NetIOSimpleUDP::SetReUseAddress(true);
	// Regular init (bind to port)
	if (!C4NetIOSimpleUDP::Init(iPort))
		return false;
	// Set callback
	C4NetIOSimpleUDP::SetCallback(this);
	// Build broadcast address
	DiscoveryAddr.sin_addr.s_addr = C4NetDiscoveryAddress;
	DiscoveryAddr.sin_port = htons(iPort);
	DiscoveryAddr.sin_family = AF_INET;
	ZeroMem(DiscoveryAddr.sin_zero, sizeof(DiscoveryAddr.sin_zero));
	// Initialize broadcast
	if (!C4NetIOSimpleUDP::InitBroadcast(&DiscoveryAddr))
		return false;
	// Enable multicast loopback
	return C4NetIOSimpleUDP::SetMCLoopback(true);
}

bool C4Network2IODiscover::Announce()
{
	// Announce our presence
	C4Network2IODiscoverReply Reply = { 4, htons(iRefServerPort) };
	return Send(C4NetIOPacket(&Reply, sizeof(Reply), false, DiscoveryAddr));
}

// *** C4Network2IODiscoverClient

void C4Network2IODiscoverClient::OnPacket(const class C4NetIOPacket &rPacket, C4NetIO *pNetIO)
{
	// discovery?
	if (rPacket.getSize() == sizeof(C4Network2IODiscoverReply) && rPacket.getStatus() == 4)
	{
		// save discovered address
		if (iDiscoverCount < C4NetMaxDiscover)
		{
			const C4Network2IODiscoverReply *pReply = reinterpret_cast<const C4Network2IODiscoverReply *>(rPacket.getData());
			Discovers[iDiscoverCount] = rPacket.getAddr();
			Discovers[iDiscoverCount].sin_port = pReply->Port;
			iDiscoverCount++;
		}
	}
}

bool C4Network2IODiscoverClient::Init(uint16_t iPort)
{
	// Reuse address
	C4NetIOSimpleUDP::SetReUseAddress(true);
	// Bind to port
	if (!C4NetIOSimpleUDP::Init(iPort))
		return false;
	// Set callback
	C4NetIOSimpleUDP::SetCallback(this);
	// Build broadcast address
	DiscoveryAddr.sin_addr.s_addr = C4NetDiscoveryAddress;
	DiscoveryAddr.sin_port = htons(iPort);
	DiscoveryAddr.sin_family = AF_INET;
	ZeroMem(DiscoveryAddr.sin_zero, sizeof(DiscoveryAddr.sin_zero));
	// Initialize broadcast
	if (!C4NetIOSimpleUDP::InitBroadcast(&DiscoveryAddr))
		return false;
	// Enable multicast loopback
	return C4NetIOSimpleUDP::SetMCLoopback(true);
}

bool C4Network2IODiscoverClient::StartDiscovery()
{
	// Multicast discovery byte
	char c = 3;
	return Send(C4NetIOPacket(&c, sizeof(c), false, DiscoveryAddr));
}

bool C4Network2IODiscoverClient::PopDiscover(C4NetIO::addr_t &Discover)
{
	// Discovers left?
	if (!getDiscoverCount())
		return false;
	// Return one
	Discover = Discovers[--iDiscoverCount];
	return true;
}
