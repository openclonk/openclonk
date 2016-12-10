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
/* Linux implementation of a UPnP port mapper (using miniupnpc) */

#include "C4Include.h"
#include "game/C4Application.h"
#include "C4Version.h"

#include <future>

#include <miniupnpc.h>
#include <upnpcommands.h>
#include <upnperrors.h>

#include "network/C4Network2UPnP.h"

static const char *description = "OpenClonk";

class C4Network2UPnPP : C4InteractiveThread
{
public:
	C4Network2UPnPP();
	virtual ~C4Network2UPnPP();

	void AddMapping(C4Network2IOProtocol protocol, uint16_t intport, uint16_t extport);
	void ClearMappings();

private:
	void Init();

	struct PortMapping {
		uint16_t external_port;
		uint16_t internal_port;
		std::string protocol;
	};

	void AddPortMapping(const PortMapping& mapping);
	void RemovePortMapping(const PortMapping& mapping);

	std::vector<PortMapping> added_mappings;

	// Synchronization using futures.
	std::future<void> action;

	bool initialized = false;
	char lanaddr[64];
	UPNPDev *devlist = nullptr;
	UPNPUrls upnp_urls;
	IGDdatas igd_data;
};

C4Network2UPnPP::C4Network2UPnPP()
{
	action = std::async(&C4Network2UPnPP::Init, this);
}

void C4Network2UPnPP::Init()
{
	int error, status;

#if MINIUPNPC_API_VERSION == 10
	// Distributed with Debian jessie.
	if ((devlist = upnpDiscover(2000, nullptr, nullptr, 0, 0, &error)))
#else
	if ((devlist = upnpDiscover(2000, nullptr, nullptr, UPNP_LOCAL_PORT_ANY, 0, 2, &error)))
#endif
	{
		if ((status = UPNP_GetValidIGD(devlist, &upnp_urls, &igd_data, lanaddr, sizeof(lanaddr))))
		{
			ThreadLogS("UPnP: Found IGD %s (status %d)", upnp_urls.controlURL, status);
			initialized = true;
		}
		else
		{
			ThreadLog("UPnP: No IGD found.");
			freeUPNPDevlist(devlist);
		}
	}
	else
	{
		ThreadLog("UPnP: No UPnP device found on the network.");
	}

}

C4Network2UPnPP::~C4Network2UPnPP()
{
	ClearMappings();
	action.wait();
	ProcessEvents(); // necessary for logging
	if (initialized)
	{
		FreeUPNPUrls(&upnp_urls);
		freeUPNPDevlist(devlist);
	}
}

void C4Network2UPnPP::AddMapping(C4Network2IOProtocol protocol, uint16_t intport, uint16_t extport)
{
	PortMapping mapping;
	mapping.external_port = extport;
	mapping.internal_port = intport;
	mapping.protocol = (protocol == P_TCP ? "TCP" : "UDP");

	added_mappings.push_back(mapping);

	action = std::async([this, action = std::move(action), mapping]() {
		action.wait();
		AddPortMapping(mapping);
	});
}

void C4Network2UPnPP::ClearMappings()
{
	action = std::async([this, action = std::move(action)]() {
		action.wait();

		for (auto mapping : added_mappings)
			RemovePortMapping(mapping);

		added_mappings.clear();
	});
}

void C4Network2UPnPP::AddPortMapping(const PortMapping& mapping)
{
	if (!initialized) return; // Catches the case that UPnP initialization failed

	auto eport = std::to_string(mapping.external_port);
	auto iport = std::to_string(mapping.internal_port);

	int r = UPNP_AddPortMapping(upnp_urls.controlURL, igd_data.first.servicetype,
			eport.c_str(), iport.c_str(), lanaddr, description,
			mapping.protocol.c_str(), 0, 0);
	if (r == UPNPCOMMAND_SUCCESS)
		ThreadLogS("UPnP: Added mapping %s %s -> %s:%s", mapping.protocol.c_str(), eport.c_str(), lanaddr, iport.c_str());
	else
		ThreadLog("UPnP: AddPortMapping failed with code %d (%s)", r, strupnperror(r));
}

void C4Network2UPnPP::RemovePortMapping(const PortMapping& mapping)
{
	if(!initialized) return; // Catches the case that UPnP initialization failed

	auto eport = std::to_string(mapping.external_port);

	int r = UPNP_DeletePortMapping(upnp_urls.controlURL, igd_data.first.servicetype,
			eport.c_str(), mapping.protocol.c_str(), 0);
	if (r == UPNPCOMMAND_SUCCESS)
		ThreadLogS("UPnP: Removed mapping %s %s", mapping.protocol.c_str(), eport.c_str());
	else
		ThreadLog("UPnP: DeletePortMapping failed with code %d (%s)", r, strupnperror(r));
}

C4Network2UPnP::C4Network2UPnP():
	p(new C4Network2UPnPP)
{
}

C4Network2UPnP::~C4Network2UPnP()
{
	delete p;
}

void C4Network2UPnP::AddMapping(C4Network2IOProtocol protocol, uint16_t intport, uint16_t extport)
{
	p->AddMapping(protocol, intport, extport);
}

void C4Network2UPnP::ClearMappings()
{
	p->ClearMappings();
}

