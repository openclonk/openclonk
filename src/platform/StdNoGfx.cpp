/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005  Peter Wortmann
 * Copyright (c) 2005  Günther Brammer
 * Copyright (c) 2011  Julius Michaelis
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
#include "C4Include.h"
#include <StdNoGfx.h>
#include <StdMeshMaterial.h>

CStdNoGfx::CStdNoGfx()
{
	Default();
}

CStdNoGfx::~CStdNoGfx()
{
	Clear();
}

bool CStdNoGfx::CreatePrimarySurfaces(bool Fullscreen, unsigned int iXRes, unsigned int iYRes, int iColorDepth, unsigned int iMonitor)
{
	Log("Graphics disabled.");
	// Save back color depth
	byByteCnt = iColorDepth / 8;
	MaxTexSize = 2147483647;
	return true;
}

bool CStdNoGfx::PrepareMaterial(StdMeshMaterial& mesh)
{
   	mesh.BestTechniqueIndex=0; return true;
}

