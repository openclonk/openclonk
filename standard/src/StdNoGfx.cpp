/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005  Peter Wortmann
 * Copyright (c) 2005  Günther Brammer
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
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
#include <Standard.h>
#include <StdNoGfx.h>

CStdNoGfx::CStdNoGfx()
	{
	Default();
	}

CStdNoGfx::~CStdNoGfx()
	{
	delete lpPrimary; lpPrimary = NULL;
	Clear();
	}

bool CStdNoGfx::CreatePrimarySurfaces(BOOL Fullscreen, unsigned int iXRes, unsigned int iYRes, int iColorDepth, unsigned int iMonitor)
	{
	Log("Graphics disabled.");
	// Save back color depth
	byByteCnt = iColorDepth / 8;
	// Create dummy surface
	lpPrimary = lpBack = new CSurface();
	MaxTexSize = 64;
	return true;
	}
