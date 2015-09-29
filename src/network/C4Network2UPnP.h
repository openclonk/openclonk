/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2012-2013, The OpenClonk Team and contributors
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
/* Interface to a UPnP port mapper */

#ifndef INC_C4Network2Upnp
#define INC_C4Network2Upnp

#include "network/C4Network2IO.h"

class C4Network2UPnP
{
	struct C4Network2UPnPP *p;
public:
	C4Network2UPnP();
	//noncopyable
	C4Network2UPnP(const C4Network2UPnP&) = delete;
	C4Network2UPnP& operator=(const C4Network2UPnP&) = delete;
	~C4Network2UPnP();

	void AddMapping(enum C4Network2IOProtocol protocol, uint16_t intport, uint16_t extport);
	void ClearMappings();
};

#endif
