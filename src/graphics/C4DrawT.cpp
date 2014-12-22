/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2011-2013, The OpenClonk Team and contributors
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
#include "C4Include.h"
#include <C4DrawT.h>
#include <StdMeshMaterial.h>

CStdNoGfx::CStdNoGfx()
{
	Default();
}

bool CStdNoGfx::CreatePrimarySurfaces(bool Fullscreen, unsigned int iXRes, unsigned int iYRes, int iColorDepth, unsigned int iMonitor)
{
	Log("Graphics disabled.");
	// Save back color depth
	byByteCnt = iColorDepth / 8;
	MaxTexSize = 2147483647;
	return true;
}

bool CStdNoGfx::PrepareMaterial(StdMeshMatManager& mat_manager, StdMeshMaterialLoader& loader, StdMeshMaterial& mat)
{
	mat.BestTechniqueIndex=0; return true;
}
