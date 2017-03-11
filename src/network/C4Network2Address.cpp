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
#ifndef _WIN32
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <net/if.h>
#endif

#include "network/C4Network2Address.h"

// *** C4Network2Address

void C4Network2Address::CompileFunc(StdCompiler *pComp)
{
	// Clear
	if (pComp->isDeserializer())
	{
		addr.Clear();
	}

	// Write protocol
	StdEnumEntry<C4Network2IOProtocol> Protocols[] =
	{
		{ "UDP", P_UDP },
		{ "TCP", P_TCP },

		{ nullptr, P_NONE },
	};
	pComp->Value(mkEnumAdaptT<uint8_t>(eProtocol, Protocols));
	pComp->Separator(StdCompiler::SEP_PART2); // ':'

	pComp->Value(mkDefaultAdapt(addr, C4NetIO::addr_t()));
}

StdStrBuf C4Network2Address::toString() const
{
	switch (eProtocol)
	{
	case P_UDP: return FormatString("UDP:%s", addr.ToString().getData());
	case P_TCP: return FormatString("TCP:%s", addr.ToString().getData());
	default:  return StdStrBuf("INVALID");
	}
}

bool C4Network2Address::operator == (const C4Network2Address &addr2) const
{
	return eProtocol == addr2.getProtocol() && addr == addr2.getAddr();
}


