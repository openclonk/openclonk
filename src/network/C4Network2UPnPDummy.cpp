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
/* Dummy implementation of a UPnP port mapper; does nothing */

#include "C4Include.h"
#include "network/C4Network2UPnP.h"

C4Network2UPnP::C4Network2UPnP() {}
C4Network2UPnP::~C4Network2UPnP() {}
void C4Network2UPnP::AddMapping(C4Network2IOProtocol, uint16_t, uint16_t) {}
void C4Network2UPnP::ClearMappings() {}
