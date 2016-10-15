/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013-2016, The OpenClonk Team and contributors
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

/* Small member of the landscape class to handle the sky background */

#ifndef INC_C4Sky
#define INC_C4Sky

#include "lib/C4Real.h"
#include "graphics/C4Shader.h"

#define C4SkyPM_Fixed     0   // sky parallax mode: fixed
#define C4SkyPM_Wind      1   // sky parallax mode: blown by the wind

class C4Sky
{
public:
	C4Sky() { Default(); }
	~C4Sky();
	void Default(); // zero fields

	bool Init(bool fSavegame, std::string names = "");
	void Clear();
	void SetColor(int32_t iIndex, int32_t iRed, int32_t iGreen, int32_t iBlue);
	void SetFadePalette(int32_t *ipColors);
	void Draw(C4TargetFacet &cgo); // draw sky
	DWORD GetSkyFadeClr(int32_t iY); // get sky color at iY
	void Execute();   // move sky
	bool SetModulation(DWORD dwWithClr, DWORD dwBackClr); // adjust the way the sky is blitted
	DWORD GetModulation(bool fBackClr)
	{ return fBackClr ? BackClr : Modulation; }
	void CompileFunc(StdCompiler *pComp);
protected:
	int32_t Width, Height;
	uint32_t Modulation;
	int32_t BackClr;          // background color behind sky
	bool BackClrEnabled;    // is the background color enabled?
public:
	class C4Surface * Surface;
	C4Shader Shader;          // shader to draw sky w/o dynamic light  (global viewport)
	C4Shader ShaderLight;     // shader to draw sky with dynamic light (player viewport)
	C4Real xdir,ydir;  // sky movement speed
	C4Real x,y;        // sky movement pos
	int32_t ParX, ParY; // parallax movement in xdir/ydir
	uint32_t FadeClr1, FadeClr2;
	int32_t ParallaxMode;     // sky scrolling mode
};

#endif
