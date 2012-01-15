/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2012  Nicolas Hake
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */
/* Interface to a UPnP port mapper */

#ifndef INC_C4Network2Upnp
#define INC_C4Network2Upnp

#include "network/C4Network2IO.h"
#include <boost/noncopyable.hpp>

class C4Network2UPnP : boost::noncopyable
{
	struct C4Network2UPnPP *p;
public:
	C4Network2UPnP();
	~C4Network2UPnP();

	void AddMapping(enum C4Network2IOProtocol protocol, uint16_t intport, uint16_t extport);
	void ClearMappings();
};

#endif
