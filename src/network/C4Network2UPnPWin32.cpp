/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2012-2016, The OpenClonk Team and contributors
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
/* Win32 implementation of a UPnP port mapper */

#include "C4Include.h"
#include "platform/C4windowswrapper.h"
#include "network/C4Network2UPnP.h"
#include "C4Version.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif
#include <natupnp.h>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#if defined(__MINGW32__) || defined(__MINGW64__)
// MinGW doesn't usually have these
extern "C" const CLSID CLSID_UPnPNAT = { 0xAE1E00AA, 0x3FD5, 0x403C, { 0x8A, 0x27, 0x2B, 0xBD, 0xC3, 0x0C, 0xD0, 0xE1 } };
extern "C" const IID IID_IUPnPNAT = { 0xB171C812, 0xCC76, 0x485A, { 0x94, 0xD8, 0xB6, 0xB3, 0xA2, 0x79, 0x4E, 0x99 } };
#endif

namespace
{
	static BSTR PROTO_UDP = ::SysAllocString(L"UDP");
	static BSTR PROTO_TCP = ::SysAllocString(L"TCP");

	template<class T> inline void SafeRelease(T* &t)
	{
		if (t) t->Release();
		t = nullptr;
	}
}

class C4Network2UPnPP
{
public:
	bool MustReleaseCOM;

	// NAT
	IStaticPortMappingCollection *mappings;
	std::set<IStaticPortMapping*> added_mappings;

	C4Network2UPnPP()
		: MustReleaseCOM(false),
		mappings(nullptr)
	{}

	void AddMapping(C4Network2IOProtocol protocol, uint16_t intport, uint16_t extport);
	void RemoveMapping(C4Network2IOProtocol protocol, uint16_t extport);
	void ClearNatMappings();
};

C4Network2UPnP::C4Network2UPnP()
	: p(new C4Network2UPnPP)
{
	Log("UPnP init...");
	// Make sure COM is available
	if (FAILED(CoInitializeEx(0, COINIT_APARTMENTTHREADED)))
	{
		// Didn't work, don't do UPnP then
		Log("UPnP fail (no COM).");
		return;
	}
	p->MustReleaseCOM = true;

	// Get the NAT service
	IUPnPNAT *nat = nullptr;
	if (FAILED(CoCreateInstance(CLSID_UPnPNAT, nullptr, CLSCTX_INPROC_SERVER, IID_IUPnPNAT, reinterpret_cast<void**>(&nat))))
	{
		Log("UPnP fail (no service).");
		return;
	}

	// Fetch NAT mappings
	for (int ctr = 0; ctr < 10; ++ctr)
	{
		// Usually it doesn't work on the first try, give Windows some time to query the IGD
		if (SUCCEEDED(nat->get_StaticPortMappingCollection(&p->mappings)) && p->mappings)
		{
			LogF("UPnP: Got NAT port mapping table after %d tries", ctr+1);
			break;
		}
		if (ctr == 2) Log(LoadResStr("IDS_MSG_UPNPHINT"));
		Sleep(1000);
	}

	SafeRelease(nat);

	if (!p->mappings) Log("UPnP fail (no mapping).");
}

C4Network2UPnP::~C4Network2UPnP()
{
	p->ClearNatMappings();
	if (p->MustReleaseCOM)
	{
		// Decrement COM reference count
		CoUninitialize();
	}
	delete p; p = nullptr;
}

void C4Network2UPnP::AddMapping(C4Network2IOProtocol protocol, uint16_t intport, uint16_t extport)
{
	p->AddMapping(protocol, intport, extport);
}

void C4Network2UPnP::ClearMappings()
{
	p->ClearNatMappings();
}

void C4Network2UPnPP::ClearNatMappings()
{
	if (!mappings)
		return;
	for(IStaticPortMapping *mapping: added_mappings)
	{
		BSTR proto, client;
		long intport, extport;
		mapping->get_ExternalPort(&extport);
		mapping->get_InternalPort(&intport);
		mapping->get_InternalClient(&client);
		mapping->get_Protocol(&proto);
		if (SUCCEEDED(mappings->Remove(extport, proto)))
			LogF("UPnP: Closed port %d->%s:%d (%s)", (int)extport, StdStrBuf(client).getData(), (int)intport, StdStrBuf(proto).getData());
		::SysFreeString(proto);
		::SysFreeString(client);
		SafeRelease(mapping);
	}
	SafeRelease(mappings);
}

void C4Network2UPnPP::AddMapping(C4Network2IOProtocol protocol, uint16_t intport, uint16_t extport)
{
	if (mappings)
	{
		// Get (one of the) local host address(es)
		char hostname[MAX_PATH];
		hostent *host;
		if (gethostname(hostname, MAX_PATH) == 0 && (host = gethostbyname(hostname)) != nullptr)	
		{
			in_addr addr;
			addr.s_addr = *(ULONG*)host->h_addr_list[0];

			BSTR description = ::SysAllocString(ADDL(C4ENGINECAPTION));
			BSTR client = ::SysAllocString(GetWideChar(inet_ntoa(addr)));
			IStaticPortMapping *mapping = nullptr;
			if (SUCCEEDED(mappings->Add(extport, protocol == P_TCP ? PROTO_TCP : PROTO_UDP, intport, client, VARIANT_TRUE, description, &mapping)))
			{
				LogF("UPnP: Successfully opened port %d->%s:%d (%s)", extport, StdStrBuf(client).getData(), intport, protocol == P_TCP ? "TCP" : "UDP");
				added_mappings.insert(mapping);
			}
			::SysFreeString(description);
			::SysFreeString(client);
		}
	}
}
