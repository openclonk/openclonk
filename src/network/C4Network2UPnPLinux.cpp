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

#include <miniupnpc.h>
#include <upnpcommands.h>
#include <upnperrors.h>

#include "network/C4Network2UPnP.h"

static const char *description = "OpenClonk";

class C4Network2UPnPP
{
public:
	C4Network2UPnPP();
	virtual ~C4Network2UPnPP();

	void AddMapping(C4Network2IOProtocol protocol, uint16_t intport, uint16_t extport);
	void ClearMappings();

private:
	struct PortMapping {
		uint16_t external_port;
		uint16_t internal_port;
		std::string protocol;
	};

	void AddPortMapping(const PortMapping& mapping);
	void RemovePortMapping(const PortMapping& mapping);

	std::vector<PortMapping> added_mappings;

	bool initialized = false;
	char lanaddr[64];
	UPNPDev *devlist = nullptr;
	UPNPUrls upnp_urls;
	IGDdatas igd_data;
};

C4Network2UPnPP::C4Network2UPnPP()
{
	int error, status;

	if ((devlist = upnpDiscover(2000, NULL, NULL, UPNP_LOCAL_PORT_ANY, 0, 2, &error)))
	{
		if ((status = UPNP_GetValidIGD(devlist, &upnp_urls, &igd_data, lanaddr, sizeof(lanaddr))))
		{
			LogF("UPnP: Found IGD %s (status %d)", upnp_urls.controlURL, status);
			initialized = true;
		}
		else
		{
			Log("UPnP: No IGD found.");
		}
	}
	else
	{
		Log("UPnP: No UPnP device found on the network.");
	}

}

C4Network2UPnPP::~C4Network2UPnPP()
{
	ClearMappings();
	FreeUPNPUrls(&upnp_urls);
	freeUPNPDevlist(devlist);
}

void C4Network2UPnPP::AddMapping(C4Network2IOProtocol protocol, uint16_t intport, uint16_t extport)
{
	PortMapping mapping;
	mapping.external_port = extport;
	mapping.internal_port = intport;
	mapping.protocol = (protocol == P_TCP ? "TCP" : "UDP");

	added_mappings.push_back(mapping);

	AddPortMapping(mapping);
}

void C4Network2UPnPP::ClearMappings()
{
	for (auto mapping : added_mappings)
		RemovePortMapping(mapping);

	added_mappings.clear();
}

void C4Network2UPnPP::AddPortMapping(const PortMapping& mapping)
{
	if (!initialized) return; // Catches the case that UPnP initialization failed

	auto eport = std::to_string(mapping.external_port);
	auto iport = std::to_string(mapping.internal_port);

	int r = UPNP_AddPortMapping(upnp_urls.controlURL, igd_data.first.servicetype,
			eport.c_str(), iport.c_str(), lanaddr, description,
			mapping.protocol.c_str(), 0, 0);
	if (r != UPNPCOMMAND_SUCCESS)
		LogF("UPnP: AddPortMapping failed with code %d (%s)", r, strupnperror(r));
}

void C4Network2UPnPP::RemovePortMapping(const PortMapping& mapping)
{
	if(!initialized) return; // Catches the case that UPnP initialization failed

	auto eport = std::to_string(mapping.external_port);

	int r = UPNP_DeletePortMapping(upnp_urls.controlURL, igd_data.first.servicetype,
			eport.c_str(), mapping.protocol.c_str(), 0);
	if (r != UPNPCOMMAND_SUCCESS)
		LogF("UPnP: DeletePortMapping failed with code %d (%s)", r, strupnperror(r));
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

