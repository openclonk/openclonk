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
#include <array>

#include "graphics/CSurface8.h"
#include "landscape/C4Landscape.h"
#include "landscape/C4Texture.h"

/* This is a small part of the implementation of C4Landscape for what is
 * required by mape. We cannot link the full implementation since it would
 * introduce a dependency on C4Game, and therefore the rest of the engine. */

struct C4Landscape::P
{
	int32_t Pix2Mat[C4M_MaxTexIndex], Pix2Dens[C4M_MaxTexIndex], Pix2Place[C4M_MaxTexIndex];
	bool Pix2Light[C4M_MaxTexIndex];
	mutable std::array<std::unique_ptr<uint8_t[]>, C4M_MaxTexIndex> BridgeMatConversion;
	int32_t Width = 0, Height = 0;
	std::unique_ptr<CSurface8> Surface8;
};

C4Landscape::C4Landscape() : p(new P) {}
C4Landscape::~C4Landscape() {}
bool C4Landscape::FindMatSlide(int&, int&, int, int, int) const { return false; }
int32_t C4Landscape::ExtractMaterial(int32_t, int32_t, bool) { return 0; }
bool C4Landscape::InsertMaterial(int32_t, int32_t *, int32_t *, int32_t, int32_t, bool) { return false; }
bool C4Landscape::Incinerate(int32_t, int32_t, int32_t) { return false; }
bool C4Landscape::ClearPix(int32_t, int32_t) { return false; }
void C4Landscape::CheckInstabilityRange(int32_t, int32_t) {}

int32_t C4Landscape::GetDensity(int32_t x, int32_t y) const { return p->Pix2Dens[GetPix(x, y)]; }
int32_t C4Landscape::GetPixDensity(BYTE byPix) const { return p->Pix2Dens[byPix]; }
C4Real C4Landscape::GetGravity() const { return C4REAL100(20); }
int32_t C4Landscape::GetMat(int32_t x, int32_t y) const { return p->Pix2Mat[GetPix(x, y)]; }

BYTE C4Landscape::GetPix(int32_t x, int32_t y) const // get landscape pixel (bounds checked)
{
	extern BYTE MCVehic;
	// Border checks
	if (x < 0 || x >= p->Width) return MCVehic;
	if (y < 0 || y >= p->Height) return MCVehic;
	return p->Surface8->_GetPix(x, y);
}

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
	for (i = 0; i < C4M_MaxTexIndex; i++) p->Pix2Mat[i] = PixCol2Mat(i);
	for (i = 0; i < C4M_MaxTexIndex; i++) p->Pix2Dens[i] = MatDensity(p->Pix2Mat[i]);
	for (i = 0; i < C4M_MaxTexIndex; i++) p->Pix2Place[i] = MatValid(p->Pix2Mat[i]) ? ::MaterialMap.Map[p->Pix2Mat[i]].Placement : 0;
	for (i = 0; i < C4M_MaxTexIndex; i++) p->Pix2Light[i] = MatValid(p->Pix2Mat[i]) && (::MaterialMap.Map[p->Pix2Mat[i]].Light>0);
	p->Pix2Place[0] = 0;
	// clear bridge mat conversion buffers
	std::fill(p->BridgeMatConversion.begin(), p->BridgeMatConversion.end(), nullptr);
}
