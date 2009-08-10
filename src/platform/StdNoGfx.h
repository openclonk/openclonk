/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005  Peter Wortmann
 * Copyright (c) 2007  Günther Brammer
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
/* Implemention of NewGfx - without gfx */

#ifndef INC_StdNoGfx
#define INC_StdNoGfx

#include <StdDDraw2.h>

class CStdNoGfx : public CStdDDraw
{
public:
	CStdNoGfx();
	virtual ~CStdNoGfx();
public:
	virtual bool PageFlip(RECT *pSrcRt=NULL, RECT *pDstRt=NULL, CStdWindow * pWindow = NULL) { return true; }
	virtual bool BeginScene() { return true; }
	virtual void EndScene() { }
	virtual int GetEngine() { return GFXENGN_NOGFX; }
	virtual void TaskOut() { }
	virtual void TaskIn() { }
	virtual bool UpdateClipper() { return true; }
	virtual bool OnResolutionChanged(unsigned int, unsigned int) { return true; }
	virtual bool PrepareRendering(SURFACE) { return true; }
	virtual void FillBG(DWORD dwClr=0) { }
	virtual void PerformBlt(CBltData &, CTexRef *, DWORD, bool, bool) { }
	virtual void PerformLine(SURFACE, float, float, float, float, DWORD) { }
	virtual void DrawQuadDw(SURFACE, float *, DWORD, DWORD, DWORD, DWORD) { }
	virtual void PerformPix(SURFACE, float, float, DWORD) { }
	virtual bool ApplyGammaRamp(D3DGAMMARAMP &, bool ) { return true; }
	virtual bool SaveDefaultGammaRamp(CStdWindow *) { return true; }
	virtual void SetTexture() { }
	virtual void ResetTexture() { }
	virtual bool InitDeviceObjects() { return true; }
	virtual bool RestoreDeviceObjects() { return true; }
	virtual bool InvalidateDeviceObjects() { return true; }
	virtual bool DeleteDeviceObjects() { return true; }
	virtual bool DeviceReady() { return true; }
	virtual bool CreatePrimarySurfaces(BOOL, unsigned int, unsigned int, int, unsigned int);
	virtual bool SetOutputAdapter(unsigned int) { return true; }
};

#endif
