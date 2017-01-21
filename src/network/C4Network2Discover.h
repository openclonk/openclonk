/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013-2016, The OpenClonk Team and contributors
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
#ifndef INC_C4Network2Discover
#define INC_C4Network2Discover

#include "network/C4NetIO.h"

const int C4NetMaxDiscover = 64;

const StdStrBuf C4NetDiscoveryAddress = StdStrBuf("ff02::1");

class C4Network2IODiscover : public C4NetIOSimpleUDP, private C4NetIO::CBClass
{
public:
	C4Network2IODiscover(uint16_t iRefServerPort) : iRefServerPort(iRefServerPort), fEnabled(false)
	{ C4NetIOSimpleUDP::SetCallback(this); }

protected:

	// callbacks (will handle everything here)
	virtual void OnPacket(const class C4NetIOPacket &rPacket, C4NetIO *pNetIO);

public:
	bool Init(uint16_t iPort = C4NetIO::addr_t::IPPORT_NONE);
	void SetDiscoverable(bool fnEnabled) { fEnabled = fnEnabled; }
	bool Announce();

private:
	C4NetIO::addr_t DiscoveryAddr;

	uint16_t iRefServerPort;
	bool fEnabled;
};

class C4Network2IODiscoverClient : public C4NetIOSimpleUDP, private C4NetIO::CBClass
{
public:
	C4Network2IODiscoverClient() : iDiscoverCount(0)
	{ C4NetIOSimpleUDP::SetCallback(this); }

protected:

	// callbacks (will handle everything here)
	virtual void OnPacket(const class C4NetIOPacket &rPacket, C4NetIO *pNetIO);

public:
	int getDiscoverCount() const { return iDiscoverCount; }
	const C4NetIO::addr_t &getDiscover(int i) { return Discovers[i]; }

	void Clear() { iDiscoverCount = 0; }
	bool Init(uint16_t iPort = C4NetIO::addr_t::IPPORT_NONE);
	bool StartDiscovery();
	bool PopDiscover(C4NetIO::addr_t &Discover);

private:
	C4NetIO::addr_t DiscoveryAddr;

	int iDiscoverCount;
	C4NetIO::addr_t Discovers[C4NetMaxDiscover];

};

#endif // INC_C4Network2Discover
