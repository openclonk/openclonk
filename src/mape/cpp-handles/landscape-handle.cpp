/*
 * mape - C4 Landscape.txt editor
 *
 * Copyright (c) 2005-2009, Armin Burgmeier
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

#include "C4Landscape.h"
#include "C4Texture.h"

/* This is a small part of the implementation of C4Landscape for what is
 * required by mape. We cannot link the full implementation since it would
 * introduce a dependency on C4Game, and therefore the rest of the engine. */
int32_t PixCol2Mat(BYTE pixc)
{
        // Get texture 
        int32_t iTex = PixCol2Tex(pixc);
        if (!iTex) return MNone;
        // Get material-texture mapping
        const C4TexMapEntry *pTex = ::TextureMap.GetEntry(iTex);
        // Return material
        return pTex ? pTex->GetMaterialIndex() : MNone;
}

void C4Landscape::HandleTexMapUpdate()
{
	UpdatePixMaps();
}

void C4Landscape::UpdatePixMaps() // Copied from C4Landscape.cpp
{
	int32_t i;
	for (i = 0; i < C4M_MaxTexIndex; i++) Pix2Mat[i] = PixCol2Mat(i);
	for (i = 0; i < C4M_MaxTexIndex; i++) Pix2Dens[i] = MatDensity(Pix2Mat[i]);
	for (i = 0; i < C4M_MaxTexIndex; i++) Pix2Place[i] = MatValid(Pix2Mat[i]) ? ::MaterialMap.Map[Pix2Mat[i]].Placement : 0;
	for (i = 0; i < C4M_MaxTexIndex; i++) Pix2Light[i] = MatValid(Pix2Mat[i]) && (::MaterialMap.Map[Pix2Mat[i]].Light>0);
	Pix2Place[0] = 0;
	// clear bridge mat conversion buffers
	for (int32_t i = 0; i < C4M_MaxTexIndex; ++i)
	{
		delete [] BridgeMatConversion[i];
		BridgeMatConversion[i] = NULL;
	}
}
