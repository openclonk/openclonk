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
/* Dummy implementation of a UPnP port mapper; does nothing */

#include "C4Include.h"
#include "network/C4Network2UPnP.h"

C4Network2UPnP::C4Network2UPnP() {}
C4Network2UPnP::~C4Network2UPnP() {}
void C4Network2UPnP::AddMapping(C4Network2IOProtocol, uint16_t, uint16_t) {}
void C4Network2UPnP::ClearMappings() {}
