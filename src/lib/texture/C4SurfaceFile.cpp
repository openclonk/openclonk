/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2002  Sven Eberhardt
 * Copyright (c) 2006-2007  Günther Brammer
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

/* Another C4Group bitmap-to-surface loader and saver */

#include <C4Include.h>
#include <C4SurfaceFile.h>

#include <C4Surface.h>
#include <C4Group.h>

C4Surface *GroupReadSurface(CStdStream &hGroup, BYTE *bpPalette)
	{
	// create surface
	C4Surface *pSfc=new C4Surface();
	if (!pSfc->ReadBMP(hGroup, !!bpPalette))
		{ delete pSfc; return NULL; }
	return pSfc;
	}

CSurface8 *GroupReadSurface8(CStdStream &hGroup)
	{
	// create surface
	CSurface8 *pSfc=new CSurface8();
	if (!pSfc->Read(hGroup, false))
		{ delete pSfc; return NULL; }
	return pSfc;
	}

C4Surface *GroupReadSurfaceOwnPal(CStdStream &hGroup)
	{
	// create surface
	C4Surface *pSfc=new C4Surface();
	if (!pSfc->ReadBMP(hGroup, true))
		{ delete pSfc; return NULL; }
	return pSfc;
	}

CSurface8 *GroupReadSurfaceOwnPal8(CStdStream &hGroup)
	{
	// create surface
	CSurface8 *pSfc=new CSurface8();
	if (!pSfc->Read(hGroup, true))
		{ delete pSfc; return NULL; }
	return pSfc;
	}
