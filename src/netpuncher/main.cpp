/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2016, The OpenClonk Team and contributors
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
#include "netpuncher/C4PuncherHash.h"
#include "netpuncher/C4PuncherPacket.h"
#include "network/C4Network2.h"

#include <random>
#include <unordered_map>

class C4PuncherServer : public C4NetIOUDP, private C4NetIO::CBClass
{
public:
	typedef C4NetpuncherID::value CID;
	C4PuncherServer() {
		C4NetIOUDP::SetCallback(this);
		rng = std::bind(std::uniform_int_distribution<CID>(1/*, max*/), std::ref(random_device));
	}
private:
	std::random_device random_device;
	std::function<CID()> rng;
	std::unordered_map<addr_t, CID> peer_ids;
	std::unordered_map<CID, addr_t> peer_addrs;
	// Event handlers
	bool OnConn(const addr_t &AddrPeer, const addr_t &AddrConnect, const addr_t *OwnAddr, C4NetIO *pNetIO) override {
		CID nid;
		do {
			nid = rng();
		} while(peer_addrs.count(nid) && !nid);
		peer_ids.emplace(AddrPeer, nid);
		peer_addrs.emplace(nid, AddrPeer);
		printf("Punched %s... #%u\n", AddrPeer.ToString().getData(), nid);
		return true;
	}
	void OnPacket(const class C4NetIOPacket &rPacket, C4NetIO *pNetIO) override {
		auto& addr = rPacket.getAddr();
		auto unpack = C4NetpuncherPacket::Construct(rPacket);
		if (!unpack) { Close(addr); return; }
		switch (unpack->GetType()) {
		case PID_Puncher_IDReq: {
			auto it = peer_ids.find(addr);
			if (it != peer_ids.end()) {
				Send(C4NetpuncherPacketAssID(it->second).PackTo(addr));
				printf("Host: #%u\n", it->second);
			}
			break;
		}
		case PID_Puncher_SReq: {
			auto other_it = peer_addrs.find(dynamic_cast<C4NetpuncherPacketSReq*>(unpack.get())->GetID());
			if (other_it == peer_addrs.end()) return; // Might be nice to return some kind of error, for purposes of debugging.
			Send(C4NetpuncherPacketCReq(other_it->second).PackTo(addr));
			Send(C4NetpuncherPacketCReq(addr).PackTo(other_it->second));
			break;
		}
		default:
			Close(addr);
		}
	}
	void OnDisconn(const addr_t &AddrPeer, C4NetIO *pNetIO, const char *szReason) override {
		auto it = peer_ids.find(AddrPeer);
		if (it == peer_ids.end()) {
			printf("ERROR: closing connection for %s: (%s) but no connection is known\n", AddrPeer.ToString().getData(), szReason);
			return;
		}
		peer_addrs.erase(it->second);
		peer_ids.erase(it);
		printf("Stopped punching %s: %s...\n", AddrPeer.ToString().getData(), szReason);
	};
} Puncher;

int main(int argc, char * argv[])
{
	// Log
	printf("Starting puncher...\n");

	// Get port
	uint16_t iPort = C4NetStdPortPuncher;
	if (argc == 2)
	{
		iPort = strtoul(argv[1], nullptr, 10);
		if (!iPort) iPort = C4NetStdPortPuncher;
	}

	// Initialize
	if (!Puncher.Init(iPort))
	{
		fprintf(stderr, "Could not initialize puncher: %s", Puncher.GetError());
		return 1;
	}

	// Log
	printf("Listening on port %d...\n", iPort);

	// Execute forever
	for (;;)
	{
		Puncher.ExecuteUntil(-1);
		fprintf(stderr, "ERROR: %s\n", Puncher.GetError());
	}

	return 0;
}

// Necessary to satisfy the linker.
void RecordRandom(uint32_t range, uint32_t val) {}
