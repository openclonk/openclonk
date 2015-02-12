/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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

/* NewGfx interfaces */
#include "C4Include.h"
#include <C4Draw.h>

#include "C4App.h"
#include <C4FontLoader.h>
#include <C4Window.h>
#include <C4DrawGL.h>
#include <C4DrawT.h>
#include <C4Markup.h>
#include "C4Rect.h"
#include <C4Config.h>
#include "StdMesh.h"
#include <CSurface8.h>

#include <stdio.h>

// Instruct Optimus laptops to use nVidia GPU instead of integrated GPU
#if defined(_WIN32) && !defined(USE_CONSOLE)
extern "C" {
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
#endif

// Global access pointer
C4Draw *pDraw=NULL;

// Transformation matrix to convert meshes from Ogre to Clonk coordinate system
const StdMeshMatrix C4Draw::OgreToClonk = StdMeshMatrix::Scale(-1.0f, 1.0f, 1.0f) * StdMeshMatrix::Rotate(float(M_PI)/2.0f, 1.0f, 0.0f, 0.0f) * StdMeshMatrix::Rotate(float(M_PI)/2.0f, 0.0f, 0.0f, 1.0f);

inline DWORD GetTextShadowClr(DWORD dwTxtClr)
{
	return RGBA(((dwTxtClr >>  0) % 256) / 3, ((dwTxtClr >>  8) % 256) / 3, ((dwTxtClr >> 16) % 256) / 3, (dwTxtClr >> 24) % 256);
}

void C4BltTransform::SetRotate(float iAngle, float fOffX, float fOffY) // set by angle and rotation offset
{
	// iAngle is in degrees (cycling from 0 to 360)
	// determine sine and cos of reversed angle in radians
	// fAngle = -iAngle * pi/180 = iAngle * -pi/180
	float fAngle = iAngle * -0.0174532925f;
	float fsin = sinf(fAngle); float fcos = cosf(fAngle);
	// set matrix values
	mat[0] = +fcos; mat[1] = +fsin; mat[2] = (1-fcos)*fOffX - fsin*fOffY;
	mat[3] = -fsin; mat[4] = +fcos; mat[5] = (1-fcos)*fOffY + fsin*fOffX;
	mat[6] = 0; mat[7] = 0; mat[8] = 1;
	/*    calculation of rotation matrix:
	  x2 = fcos*(x1-fOffX) + fsin*(y1-fOffY) + fOffX
	     = fcos*x1 - fcos*fOffX + fsin*y1 - fsin*fOffY + fOffX
	     = x1*fcos + y1*fsin + (1-fcos)*fOffX - fsin*fOffY

	  y2 = -fsin*(x1-fOffX) + fcos*(y1-fOffY) + fOffY
	     = x1*-fsin + fsin*fOffX + y1*fcos - fcos*fOffY + fOffY
	     = x1*-fsin + y1*fcos + fsin*fOffX + (1-fcos)*fOffY */
}

bool C4BltTransform::SetAsInv(C4BltTransform &r)
{
	// calc inverse of matrix
	float det = r.mat[0]*r.mat[4]*r.mat[8] + r.mat[1]*r.mat[5]*r.mat[6]
	            + r.mat[2]*r.mat[3]*r.mat[7] - r.mat[2]*r.mat[4]*r.mat[6]
	            - r.mat[0]*r.mat[5]*r.mat[7] - r.mat[1]*r.mat[3]*r.mat[8];
	if (!det) { Set(1,0,0,0,1,0,0,0,1); return false; }
	mat[0] = (r.mat[4] * r.mat[8] - r.mat[5] * r.mat[7]) / det;
	mat[1] = (r.mat[2] * r.mat[7] - r.mat[1] * r.mat[8]) / det;
	mat[2] = (r.mat[1] * r.mat[5] - r.mat[2] * r.mat[4]) / det;
	mat[3] = (r.mat[5] * r.mat[6] - r.mat[3] * r.mat[8]) / det;
	mat[4] = (r.mat[0] * r.mat[8] - r.mat[2] * r.mat[6]) / det;
	mat[5] = (r.mat[2] * r.mat[3] - r.mat[0] * r.mat[5]) / det;
	mat[6] = (r.mat[3] * r.mat[7] - r.mat[4] * r.mat[6]) / det;
	mat[7] = (r.mat[1] * r.mat[6] - r.mat[0] * r.mat[7]) / det;
	mat[8] = (r.mat[0] * r.mat[4] - r.mat[1] * r.mat[3]) / det;
	return true;
}

void C4BltTransform::TransformPoint(float &rX, float &rY) const
{
	// apply matrix
	float fW = mat[6] * rX + mat[7] * rY + mat[8];
	// store in temp, so original rX is used for calculation of rY
	float fX = (mat[0] * rX + mat[1] * rY + mat[2]) / fW;
	rY = (mat[3] * rX + mat[4] * rY + mat[5]) / fW;
	rX = fX; // apply temp
}

C4Pattern& C4Pattern::operator=(const C4Pattern& nPattern)
{
	sfcPattern32 = nPattern.sfcPattern32;
	if (sfcPattern32) sfcPattern32->Lock();
	delete [] CachedPattern;
	if (nPattern.CachedPattern)
	{
		CachedPattern = new uint32_t[sfcPattern32->Wdt * sfcPattern32->Hgt];
		memcpy(CachedPattern, nPattern.CachedPattern, sfcPattern32->Wdt * sfcPattern32->Hgt * 4);
	}
	else
	{
		CachedPattern = 0;
	}
	Wdt = nPattern.Wdt;
	Hgt = nPattern.Hgt;
	Zoom = nPattern.Zoom;
	return *this;
}

bool C4Pattern::Set(C4Surface * sfcSource, int iZoom)
{
	// Safety
	if (!sfcSource) return false;
	// Clear existing pattern
	Clear();
	// new style: simply store pattern for modulation or shifting, which will be decided upon use
	sfcPattern32=sfcSource;
	sfcPattern32->Lock();
	Wdt = sfcPattern32->Wdt;
	Hgt = sfcPattern32->Hgt;
	// set zoom
	Zoom=iZoom;
	// set flags
	CachedPattern = new uint32_t[Wdt * Hgt];
	if (!CachedPattern) return false;
	for (int y = 0; y < Hgt; ++y)
		for (int x = 0; x < Wdt; ++x)
		{
			CachedPattern[y * Wdt + x] = sfcPattern32->GetPixDw(x, y, false);
		}
	return true;
}

C4Pattern::C4Pattern()
{
	// disable
	sfcPattern32=NULL;
	CachedPattern = 0;
	Zoom=0;
}

void C4Pattern::Clear()
{
	// pattern assigned
	if (sfcPattern32)
	{
		// unlock it
		sfcPattern32->Unlock();
		// clear field
		sfcPattern32=NULL;
	}
	delete[] CachedPattern; CachedPattern = 0;
}

DWORD C4Pattern::PatternClr(unsigned int iX, unsigned int iY) const
{
	if (!CachedPattern) return 0;
	// wrap position
	iX %= Wdt; iY %= Hgt;
	return CachedPattern[iY * Wdt + iX];
}

void C4GammaControl::SetClrChannel(WORD *pBuf, BYTE c1, BYTE c2, int c3)
{
	// Using this minimum value, gamma ramp errors on some cards can be avoided
	int MinGamma = 0x100;
	// adjust clr3-value
	++c3;
	// get rises
	int r1=c2-c1,r2=c3-c2,r=(c3-c1)/2;
	// calc beginning and end rise
	r1=2*r1-r; r2=2*r2-r;
	// calc ramp
	WORD *pBuf2=pBuf+128;
	for (int i=0; i<128; ++i)
	{
		int i2=128-i;
		// interpolate linear ramps with the rises r1 and r
		*pBuf ++=Clamp(((c1+r1*i/128) *i2  +  (c2-r*i2/128) *i) <<1, MinGamma, 0xffff);
		// interpolate linear ramps with the rises r and r2
		*pBuf2++=Clamp(((c2+r*i/128) *i2  +  (c3-r2*i2/128) *i) <<1, MinGamma, 0xffff);
	}
}

void C4GammaControl::Set(DWORD dwClr1, DWORD dwClr2, DWORD dwClr3)
{
	// set red, green and blue channel
	SetClrChannel(ramp.red  , GetRedValue(dwClr1), GetRedValue(dwClr2), GetRedValue(dwClr3));
	SetClrChannel(ramp.green, GetGreenValue(dwClr1), GetGreenValue(dwClr2), GetGreenValue(dwClr3));
	SetClrChannel(ramp.blue , GetBlueValue(dwClr1), GetBlueValue(dwClr2), GetBlueValue(dwClr3));
}

DWORD C4GammaControl::ApplyTo(DWORD dwClr)
{
	// apply to red, green and blue color component
	return RGBA(ramp.red[GetRedValue(dwClr)]>>8, ramp.green[GetGreenValue(dwClr)]>>8, ramp.blue[GetBlueValue(dwClr)]>>8, dwClr>>24);
}


//--------------------------------------------------------------------

void C4Draw::Default()
{
	RenderTarget=NULL;
	ClipAll=false;
	Active=false;
	BlitModulated=false;
	dwBlitMode = 0;
	Gamma.Default();
	DefRamp.Default();
	pFoW = NULL;
	ZoomX = 0; ZoomY = 0; Zoom = 1;
	MeshTransform = NULL;
	fUsePerspective = false;
	for (int32_t iRamp=0; iRamp<3*C4MaxGammaRamps; iRamp+=3)
		{ dwGamma[iRamp+0]=0; dwGamma[iRamp+1]=0x808080; dwGamma[iRamp+2]=0xffffff; }
	fSetGamma=false;
}

void C4Draw::Clear()
{
	ResetGamma();
	DisableGamma();
	Active=BlitModulated=false;
	dwBlitMode = 0;
}

bool C4Draw::GetSurfaceSize(C4Surface * sfcSurface, int &iWdt, int &iHgt)
{
	return sfcSurface->GetSurfaceSize(iWdt, iHgt);
}

bool C4Draw::SubPrimaryClipper(int iX1, int iY1, int iX2, int iY2)
{
	// Set sub primary clipper
	SetPrimaryClipper(Max(iX1,iClipX1),Max(iY1,iClipY1),Min(iX2,iClipX2),Min(iY2,iClipY2));
	return true;
}

bool C4Draw::StorePrimaryClipper()
{
	// Store current primary clipper
	fStClipX1=fClipX1; fStClipY1=fClipY1; fStClipX2=fClipX2; fStClipY2=fClipY2;
	return true;
}

bool C4Draw::RestorePrimaryClipper()
{
	// Restore primary clipper
	SetPrimaryClipper(fStClipX1, fStClipY1, fStClipX2, fStClipY2);
	return true;
}

bool C4Draw::SetPrimaryClipper(int iX1, int iY1, int iX2, int iY2)
{
	// set clipper
	fClipX1=iX1; fClipY1=iY1; fClipX2=iX2; fClipY2=iY2;
	iClipX1=iX1; iClipY1=iY1; iClipX2=iX2; iClipY2=iY2;
	UpdateClipper();
	// Done
	return true;
}

bool C4Draw::ApplyPrimaryClipper(C4Surface * sfcSurface)
{
	//sfcSurface->SetClipper(lpClipper);
	return true;
}

bool C4Draw::DetachPrimaryClipper(C4Surface * sfcSurface)
{
	//sfcSurface->SetClipper(NULL);
	return true;
}

bool C4Draw::NoPrimaryClipper()
{
	// apply maximum clipper
	SetPrimaryClipper(0,0,439832,439832);
	// Done
	return true;
}

void C4Draw::BlitLandscape(C4Surface * sfcSource, float fx, float fy,
                              C4Surface * sfcTarget, float tx, float ty, float wdt, float hgt)
{
	Blit(sfcSource, fx, fy, wdt, hgt, sfcTarget, tx, ty, wdt, hgt, false);
}

void C4Draw::Blit8Fast(CSurface8 * sfcSource, int fx, int fy,
                          C4Surface * sfcTarget, int tx, int ty, int wdt, int hgt)
{
	// blit 8bit-sfc
	// lock surfaces
	assert(sfcTarget->fPrimary);
	bool fRender = sfcTarget->IsRenderTarget();
	if (!fRender) if (!sfcTarget->Lock())
			{ return; }

	float tfx = tx, tfy = ty, twdt = wdt, thgt = hgt;
	if (Zoom != 1.0)
	{
		ApplyZoom(tfx, tfy);
		twdt *= Zoom;
		thgt *= Zoom;
	}

	// blit 8 bit pix in batches of 1024 pixels
	static const int BUF_SIZE = 1024;
	C4BltVertex* vertices = new C4BltVertex[BUF_SIZE];
	int bufcnt = 0;

	for (int ycnt=0; ycnt<thgt; ++ycnt)
	{
		for (int xcnt=0; xcnt<twdt; ++xcnt)
		{
			BYTE byPix = sfcSource->GetPix(fx+wdt*xcnt/twdt, fy+hgt*ycnt/thgt);
			DWORD dwClr = byPix ? sfcSource->pPal->GetClr(byPix) : 0x00000000;

			vertices[bufcnt].ftx = (float)(tx + xcnt / Zoom);
			vertices[bufcnt].fty = (float)(ty + ycnt / Zoom);
			DwTo4UB(dwClr, vertices[bufcnt].color);
			++bufcnt;

			if(bufcnt == BUF_SIZE)
			{
				PerformMultiPix(sfcTarget, vertices, BUF_SIZE);
				bufcnt = 0;
			}
		}

	}
	if(bufcnt > 0)
		PerformMultiPix(sfcTarget, vertices, bufcnt);
	delete[] vertices;
	// unlock
	if (!fRender) sfcTarget->Unlock();
}

bool C4Draw::Blit(C4Surface * sfcSource, float fx, float fy, float fwdt, float fhgt,
                     C4Surface * sfcTarget, float tx, float ty, float twdt, float thgt,
                     bool fSrcColKey, const C4BltTransform *pTransform)
{
	return BlitUnscaled(sfcSource, fx * sfcSource->Scale, fy * sfcSource->Scale, fwdt * sfcSource->Scale, fhgt * sfcSource->Scale,
	                    sfcTarget, tx, ty, twdt, thgt, fSrcColKey, pTransform);
}

bool C4Draw::BlitUnscaled(C4Surface * sfcSource, float fx, float fy, float fwdt, float fhgt,
                     C4Surface * sfcTarget, float tx, float ty, float twdt, float thgt,
                     bool fSrcColKey, const C4BltTransform *pTransform)
{
	// safety
	if (!sfcSource || !sfcTarget || !twdt || !thgt || !fwdt || !fhgt) return false;
	// emulated blit?
	if (!sfcTarget->IsRenderTarget())
	{
		C4BltTransform t;
		if(pTransform && Zoom != 1.0)
		{
			t.Set(pTransform->mat[0]*Zoom, pTransform->mat[1]*Zoom, pTransform->mat[2]*Zoom + ZoomX*(1-Zoom),
			      pTransform->mat[3]*Zoom, pTransform->mat[4]*Zoom, pTransform->mat[5]*Zoom + ZoomY*(1-Zoom),
			      pTransform->mat[6], pTransform->mat[7], pTransform->mat[8]);
			pTransform = &t;
		}
		else if(Zoom != 1.0)
		{
			ApplyZoom(tx, ty);
			twdt *= Zoom;
			thgt *= Zoom;
		}

		return Blit8(sfcSource, int(fx), int(fy), int(fwdt), int(fhgt), sfcTarget, int(tx), int(ty), int(twdt), int(thgt), fSrcColKey, pTransform);
	}

	// calc stretch
	float scaleX = twdt/fwdt;
	float scaleY = thgt/fhgt;
	// bound
	if (ClipAll) return true;
	// inside screen?
	if (twdt<=0 || thgt<=0) return false;
	// prepare rendering to surface
	if (!PrepareRendering(sfcTarget)) return false;
	// texture present?
	if (sfcSource->textures.empty())
	{
		// primary surface?
		if (sfcSource->fPrimary)
		{
			// blit emulated
			return Blit8(sfcSource, int(fx), int(fy), int(fwdt), int(fhgt), sfcTarget, int(tx), int(ty), int(twdt), int(thgt), fSrcColKey);
		}
		return false;
	}
	// blit with basesfc?
	bool fBaseSfc=false;
	if (sfcSource->pMainSfc) if (!sfcSource->pMainSfc->textures.empty()) fBaseSfc = true;
	// get involved texture offsets
	int iTexSizeX=sfcSource->iTexSize;
	int iTexSizeY=sfcSource->iTexSize;
	int iTexX=Max(int(fx/iTexSizeX), 0);
	int iTexY=Max(int(fy/iTexSizeY), 0);
	int iTexX2=Min((int)(fx+fwdt-1)/iTexSizeX +1, sfcSource->iTexX);
	int iTexY2=Min((int)(fy+fhgt-1)/iTexSizeY +1, sfcSource->iTexY);
	// calc stretch regarding texture size and indent
/*	float scaleX2 = scaleX * iTexSizeX;
	float scaleY2 = scaleY * iTexSizeY;*/
	// blit from all these textures
	for (int iY=iTexY; iY<iTexY2; ++iY)
	{
		for (int iX=iTexX; iX<iTexX2; ++iX)
		{
			C4TexRef *pTex = &sfcSource->textures[iY * sfcSource->iTexX + iX];
			// get current blitting offset in texture
			int iBlitX=sfcSource->iTexSize*iX;
			int iBlitY=sfcSource->iTexSize*iY;
			// size changed? recalc dependant, relevant (!) values
			if (iTexSizeX != pTex->iSizeX)
			{
				iTexSizeX = pTex->iSizeX;
				/*scaleX2 = scaleX * iTexSizeX;*/
			}
			if (iTexSizeY != pTex->iSizeY)
			{
				iTexSizeY = pTex->iSizeY;
				/*scaleY2 = scaleY * iTexSizeY;*/
			}

			// get new texture source bounds
			FLOAT_RECT fTexBlt;
			fTexBlt.left  = Max<float>(fx - iBlitX, 0);
			fTexBlt.top   = Max<float>(fy - iBlitY, 0);
			fTexBlt.right = Min<float>(fx + fwdt - (float)iBlitX, (float)iTexSizeX);
			fTexBlt.bottom= Min<float>(fy + fhgt - (float)iBlitY, (float)iTexSizeY);
			// get new dest bounds
			FLOAT_RECT tTexBlt;
			tTexBlt.left  = (fTexBlt.left  + iBlitX - fx) * scaleX + tx;
			tTexBlt.top   = (fTexBlt.top   + iBlitY - fy) * scaleY + ty;
			tTexBlt.right = (fTexBlt.right + iBlitX - fx) * scaleX + tx;
			tTexBlt.bottom= (fTexBlt.bottom+ iBlitY - fy) * scaleY + ty;

			// set up blit data as rect
			C4BltVertex vertices[6];
			vertices[0].ftx = tTexBlt.left;  vertices[0].fty = tTexBlt.top;
			vertices[1].ftx = tTexBlt.right; vertices[1].fty = tTexBlt.top;
			vertices[2].ftx = tTexBlt.right; vertices[2].fty = tTexBlt.bottom;
			vertices[3].ftx = tTexBlt.left;  vertices[3].fty = tTexBlt.bottom;
			vertices[0].tx = fTexBlt.left / iTexSizeX;  vertices[0].ty = fTexBlt.top / iTexSizeY;
			vertices[1].tx = fTexBlt.right / iTexSizeX; vertices[1].ty = fTexBlt.top / iTexSizeY;
			vertices[2].tx = fTexBlt.right / iTexSizeX; vertices[2].ty = fTexBlt.bottom / iTexSizeY;
			vertices[3].tx = fTexBlt.left / iTexSizeX;  vertices[3].ty = fTexBlt.bottom / iTexSizeY;
			DwTo4UB(0xffffffff, vertices[0].color);
			DwTo4UB(0xffffffff, vertices[1].color);
			DwTo4UB(0xffffffff, vertices[2].color);
			DwTo4UB(0xffffffff, vertices[3].color);

			// duplicate vertices
			vertices[4] = vertices[0]; vertices[5] = vertices[2];

			C4TexRef * pBaseTex = pTex;
			// is there a base-surface to be blitted first?
			if (fBaseSfc)
			{
				// then get this surface as same offset as from other surface
				// assuming this is only valid as long as there's no texture management,
				// organizing partially used textures together!
				pBaseTex = &sfcSource->pMainSfc->textures[iY * sfcSource->iTexX + iX];
			}

			C4TexRef* pNormalTex = NULL;
			if (sfcSource->pNormalSfc)
				pNormalTex = &sfcSource->pNormalSfc->textures[iY * sfcSource->iTexX + iX];

			// ClrByOwner is always fully opaque
			const DWORD dwOverlayClrMod = 0xff000000 | sfcSource->ClrByOwnerClr;
			PerformMultiTris(sfcTarget, vertices, 6, pTransform, pBaseTex, fBaseSfc ? pTex : NULL, pNormalTex, dwOverlayClrMod);
		}
	}
	// success
	return true;
}

bool C4Draw::RenderMesh(StdMeshInstance &instance, C4Surface * sfcTarget, float tx, float ty, float twdt, float thgt, DWORD dwPlayerColor, C4BltTransform* pTransform)
{
	// TODO: Emulate rendering
	if (!sfcTarget->IsRenderTarget()) return false;

	// TODO: Clip

	// prepare rendering to surface
	if (!PrepareRendering(sfcTarget)) return false;
	// Update bone matrices and vertex data (note this also updates attach transforms and child transforms)
	instance.UpdateBoneTransforms();
	// Order faces according to MeshTransformation (note pTransform does not affect Z coordinate, so does not need to be taken into account for correct ordering)
	StdMeshMatrix mat = OgreToClonk;
	if(MeshTransform) mat = *MeshTransform * mat;
	instance.ReorderFaces(&mat);
	// Render mesh
	PerformMesh(instance, tx, ty, twdt, thgt, dwPlayerColor, pTransform);
	// success
	return true;
}

bool C4Draw::Blit8(C4Surface * sfcSource, int fx, int fy, int fwdt, int fhgt,
                      C4Surface * sfcTarget, int tx, int ty, int twdt, int thgt,
                      bool fSrcColKey, const C4BltTransform *pTransform)
{
	if (!pTransform) return BlitSimple(sfcSource, fx, fy, fwdt, fhgt, sfcTarget, tx, ty, twdt, thgt, fSrcColKey!=false);
	// safety
	if (!fwdt || !fhgt) return true;
	// Lock the surfaces
	if (!sfcSource->Lock())
		return false;
	if (!sfcTarget->Lock())
		{ sfcSource->Unlock(); return false; }
	// transformed, emulated blit
	// Calculate transform target rect
	C4BltTransform Transform;
	Transform.SetMoveScale(tx-(float)fx*twdt/fwdt, ty-(float)fy*thgt/fhgt, (float) twdt/fwdt, (float) thgt/fhgt);
	Transform *=* pTransform;
	C4BltTransform TransformBack;
	TransformBack.SetAsInv(Transform);
	float ttx0=(float)tx, tty0=(float)ty, ttx1=(float)(tx+twdt), tty1=(float)(ty+thgt);
	float ttx2=(float)ttx0, tty2=(float)tty1, ttx3=(float)ttx1, tty3=(float)tty0;
	pTransform->TransformPoint(ttx0, tty0);
	pTransform->TransformPoint(ttx1, tty1);
	pTransform->TransformPoint(ttx2, tty2);
	pTransform->TransformPoint(ttx3, tty3);
	int ttxMin = Max<int>((int)floor(Min(Min(ttx0, ttx1), Min(ttx2, ttx3))), 0);
	int ttxMax = Min<int>((int)ceil(Max(Max(ttx0, ttx1), Max(ttx2, ttx3))), sfcTarget->Wdt);
	int ttyMin = Max<int>((int)floor(Min(Min(tty0, tty1), Min(tty2, tty3))), 0);
	int ttyMax = Min<int>((int)ceil(Max(Max(tty0, tty1), Max(tty2, tty3))), sfcTarget->Hgt);
	// blit within target rect
	for (int y = ttyMin; y < ttyMax; ++y)
		for (int x = ttxMin; x < ttxMax; ++x)
		{
			float ffx=(float)x, ffy=(float)y;
			TransformBack.TransformPoint(ffx, ffy);
			int ifx=static_cast<int>(ffx), ify=static_cast<int>(ffy);
			if (ifx<fx || ify<fy || ifx>=fx+fwdt || ify>=fy+fhgt) continue;
			sfcTarget->BltPix(x,y, sfcSource, ifx,ify, !!fSrcColKey);
		}
	// Unlock the surfaces
	sfcSource->Unlock();
	sfcTarget->Unlock();
	return true;
}

bool C4Draw::BlitSimple(C4Surface * sfcSource, int fx, int fy, int fwdt, int fhgt,
                           C4Surface * sfcTarget, int tx, int ty, int twdt, int thgt,
                           bool fTransparency)
{
	// rendertarget?
	if (sfcTarget->IsRenderTarget())
	{
		return Blit(sfcSource, float(fx), float(fy), float(fwdt), float(fhgt), sfcTarget, float(tx), float(ty), float(twdt), float(thgt), true);
	}
	// Object is first stretched to dest rect
	int xcnt,ycnt,tcx,tcy,cpcx,cpcy;
	if (!fwdt || !fhgt || !twdt || !thgt) return false;
	// Lock the surfaces
	if (!sfcSource->Lock())
		return false;
	if (!sfcTarget->Lock())
		{ sfcSource->Unlock(); return false; }
	// Rectangle centers
	tcx=twdt/2; tcy=thgt/2;
	for (ycnt=0; ycnt<thgt; ycnt++)
		if (Inside(cpcy=ty+tcy-thgt/2+ycnt,0,sfcTarget->Hgt-1))
			for (xcnt=0; xcnt<twdt; xcnt++)
				if (Inside(cpcx=tx+tcx-twdt/2+xcnt,0,sfcTarget->Wdt-1))
					sfcTarget->BltPix(cpcx, cpcy, sfcSource, xcnt*fwdt/twdt+fx, ycnt*fhgt/thgt+fy, fTransparency);
	// Unlock the surfaces
	sfcSource->Unlock();
	sfcTarget->Unlock();
	return true;
}


bool C4Draw::Error(const char *szMsg)
{
	if (pApp) pApp->Error(szMsg);
	Log(szMsg); return false;
}


bool C4Draw::CreatePrimaryClipper(unsigned int iXRes, unsigned int iYRes)
{
	// simply setup primary viewport
	// assume no zoom has been set yet
	assert(Zoom==1.0f);
	SetPrimaryClipper(0, 0, iXRes - 1, iYRes - 1);
	StorePrimaryClipper();
	return true;
}

bool C4Draw::BlitSurface(C4Surface * sfcSurface, C4Surface * sfcTarget, int tx, int ty, bool fBlitBase)
{
	if (fBlitBase)
	{
		Blit(sfcSurface, 0.0f, 0.0f, (float)sfcSurface->Wdt, (float)sfcSurface->Hgt, sfcTarget, float(tx), float(ty), float(sfcSurface->Wdt), float(sfcSurface->Hgt), false);
		return true;
	}
	else
	{
		if (!sfcSurface) return false;
		C4Surface *pSfcBase = sfcSurface->pMainSfc;
		sfcSurface->pMainSfc = NULL;
		Blit(sfcSurface, 0.0f, 0.0f, (float)sfcSurface->Wdt, (float)sfcSurface->Hgt, sfcTarget, float(tx), float(ty), float(sfcSurface->Wdt), float(sfcSurface->Hgt), false);
		sfcSurface->pMainSfc = pSfcBase;
		return true;
	}
}

bool C4Draw::BlitSurfaceTile(C4Surface * sfcSurface, C4Surface * sfcTarget, float iToX, float iToY, float iToWdt, float iToHgt, float iOffsetX, float iOffsetY, bool fSrcColKey)
{
	int iSourceWdt,iSourceHgt;
	float iX,iY,iBlitX,iBlitY,iBlitWdt,iBlitHgt;
	// Get source surface size
	if (!GetSurfaceSize(sfcSurface,iSourceWdt,iSourceHgt)) return false;
	// reduce offset to needed size
	iOffsetX = fmod(iOffsetX, iSourceWdt);
	iOffsetY = fmod(iOffsetY, iSourceHgt);
	// Vertical blits
	for (iY=iToY+iOffsetY; iY<iToY+iToHgt; iY+=iSourceHgt)
	{
		// Vertical blit size
		iBlitY=Max(iToY-iY,0.0f); iBlitHgt=Min<float>(iSourceHgt,iToY+iToHgt-iY)-iBlitY;
		// Horizontal blits
		for (iX=iToX+iOffsetX; iX<iToX+iToWdt; iX+=iSourceWdt)
		{
			// Horizontal blit size
			iBlitX=Max(iToX-iX,0.0f); iBlitWdt=Min<float>(iSourceWdt,iToX+iToWdt-iX)-iBlitX;
			// Blit
			if (!Blit(sfcSurface, iBlitX, iBlitY, iBlitWdt, iBlitHgt, sfcTarget, iX+iBlitX, iY+iBlitY, iBlitWdt, iBlitHgt, fSrcColKey)) return false;
		}
	}
	return true;
}

bool C4Draw::BlitSurfaceTile2(C4Surface * sfcSurface, C4Surface * sfcTarget, float iToX, float iToY, float iToWdt, float iToHgt, float iOffsetX, float iOffsetY, bool fSrcColKey)
{
	// if it's a render target, simply blit with repeating texture
	// repeating textures, however, aren't currently supported
	/*if (sfcTarget->IsRenderTarget())
	  return Blit(sfcSurface, iOffsetX, iOffsetY, iToWdt, iToHgt, sfcTarget, iToX, iToY, iToWdt, iToHgt, false);*/
	float tx,ty,iBlitX,iBlitY,iBlitWdt,iBlitHgt;
	// get tile size
	int iTileWdt=sfcSurface->Wdt;
	int iTileHgt=sfcSurface->Hgt;
	// adjust size of offsets
	iOffsetX = fmod(iOffsetX, iTileWdt);
	iOffsetY = fmod(iOffsetY, iTileHgt);
	if (iOffsetX<0) iOffsetX+=iTileWdt;
	if (iOffsetY<0) iOffsetY+=iTileHgt;
	// get start pos for blitting
	float iStartX=iToX-iOffsetX;
	float iStartY=iToY-iOffsetY;
	ty=0;
	// blit vertical
	for (float iY=iStartY; fabs(ty - iToHgt) > 1e-3; iY+=iTileHgt)
	{
		// get vertical blit bounds
		iBlitY=0; iBlitHgt=iTileHgt;
		if (iY<iToY) { iBlitY=iToY-iY; iBlitHgt+=iY-iToY; }
		float iOver=ty+iBlitHgt-iToHgt; if (iOver>0) iBlitHgt-=iOver;
		// blit horizontal
		tx=0;
		for (float iX=iStartX; fabs(tx - iToWdt) > 1e-3; iX+=iTileWdt)
		{
			// get horizontal blit bounds
			iBlitX=0; iBlitWdt=iTileWdt;
			if (iX<iToX) { iBlitX=iToX-iX; iBlitWdt+=iX-iToX; }
			iOver=tx+iBlitWdt-iToWdt; if (iOver>0) iBlitWdt-=iOver;
			// blit
			if (!Blit(sfcSurface,iBlitX,iBlitY,iBlitWdt,iBlitHgt,sfcTarget,tx+iToX,ty+iToY,iBlitWdt,iBlitHgt,fSrcColKey))
			{
				// Ignore blit errors. This is usually due to blit border lying outside surface and shouldn't cause remaining blits to fail.
			}
			// next col
			tx+=iBlitWdt;
		}
		// next line
		ty+=iBlitHgt;
	}
	// success
	return true;
}

bool C4Draw::TextOut(const char *szText, CStdFont &rFont, float fZoom, C4Surface * sfcDest, float iTx, float iTy, DWORD dwFCol, BYTE byForm, bool fDoMarkup)
{
	C4Markup Markup(true);
	static char szLinebuf[2500+1];
	for (int cnt=0; SCopySegmentEx(szText,cnt,szLinebuf,fDoMarkup ? '|' : '\n','\n',2500); cnt++,iTy+=int(fZoom*rFont.GetLineHeight()))
		if (!StringOut(szLinebuf,sfcDest,iTx,iTy,dwFCol,byForm,fDoMarkup,Markup,&rFont,fZoom)) return false;
	return true;
}

bool C4Draw::StringOut(const char *szText, CStdFont &rFont, float fZoom, C4Surface * sfcDest, float iTx, float iTy, DWORD dwFCol, BYTE byForm, bool fDoMarkup)
{
	// init markup
	C4Markup Markup(true);
	// output string
	return StringOut(szText, sfcDest, iTx, iTy, dwFCol, byForm, fDoMarkup, Markup, &rFont, fZoom);
}

bool C4Draw::StringOut(const char *szText, C4Surface * sfcDest, float iTx, float iTy, DWORD dwFCol, BYTE byForm, bool fDoMarkup, C4Markup &Markup, CStdFont *pFont, float fZoom)
{
	// clip
	if (ClipAll) return true;
	// safety
	if (!PrepareRendering(sfcDest)) return false;
	// convert align
	int iFlags=0;
	switch (byForm)
	{
	case ACenter: iFlags |= STDFONT_CENTERED; break;
	case ARight: iFlags |= STDFONT_RIGHTALGN; break;
	}
	if (!fDoMarkup) iFlags|=STDFONT_NOMARKUP;
	// draw text
	pFont->DrawText(sfcDest, iTx  , iTy  , dwFCol, szText, iFlags, Markup, fZoom);
	// done, success
	return true;
}

void C4Draw::DrawPix(C4Surface * sfcDest, float tx, float ty, DWORD dwClr)
{
	// Draw
	C4BltVertex vtx;
	vtx.ftx = tx;
	vtx.fty = ty;
	DwTo4UB(dwClr, vtx.color);
	PerformMultiPix(sfcDest, &vtx, 1);
}

void C4Draw::DrawLineDw(C4Surface * sfcTarget, float x1, float y1, float x2, float y2, DWORD dwClr, float width)
{
	C4BltVertex vertices[2];
	vertices[0].ftx = x1; vertices[0].fty = y1;
	vertices[1].ftx = x2; vertices[1].fty = y2;
	DwTo4UB(dwClr, vertices[0].color);
	DwTo4UB(dwClr, vertices[1].color);
	PerformMultiLines(sfcTarget, vertices, 2, width);
}

void C4Draw::DrawFrameDw(C4Surface * sfcDest, int x1, int y1, int x2, int y2, DWORD dwClr) // make these parameters float...?
{
	C4BltVertex vertices[8];
	vertices[0].ftx = x1; vertices[0].fty = y1;
	vertices[1].ftx = x2; vertices[1].fty = y1;
	vertices[2] = vertices[1];
	vertices[3].ftx = x2; vertices[3].fty = y2;
	vertices[4] = vertices[3];
	vertices[5].ftx = x1; vertices[5].fty = y2;
	vertices[6] = vertices[5];
	vertices[7] = vertices[0];

	for(int i = 0; i < 8; ++i)
		DwTo4UB(dwClr, vertices[i].color);

	PerformMultiLines(sfcDest, vertices, 8, 1.0f);
}

void C4Draw::DrawQuadDw(C4Surface * sfcTarget, float *ipVtx, DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, DWORD dwClr4)
{
	C4BltVertex vertices[6];
	vertices[0].ftx = ipVtx[0]; vertices[0].fty = ipVtx[1];
	vertices[1].ftx = ipVtx[2]; vertices[1].fty = ipVtx[3];
	vertices[2].ftx = ipVtx[4]; vertices[2].fty = ipVtx[5];
	vertices[3].ftx = ipVtx[6]; vertices[3].fty = ipVtx[7];
	DwTo4UB(dwClr1, vertices[0].color);
	DwTo4UB(dwClr2, vertices[1].color);
	DwTo4UB(dwClr3, vertices[2].color);
	DwTo4UB(dwClr4, vertices[3].color);
	vertices[4] = vertices[0];
	vertices[5] = vertices[2];
	PerformMultiTris(sfcTarget, vertices, 6, NULL, NULL, NULL, NULL, 0);
}

void C4Draw::DrawPatternedCircle(C4Surface * sfcDest, int x, int y, int r, BYTE col, C4Pattern & Pattern, CStdPalette &rPal)
{
	bool fRenderTarget = sfcDest->IsRenderTarget();
	if (!fRenderTarget) if (!sfcDest->Lock()) return;
	for (int ycnt = -r; ycnt < r; ycnt++)
	{
		int lwdt = (int)sqrt(float(r * r - ycnt * ycnt));
		// Set line
		if (fRenderTarget)
		{
			for (int xcnt = x - lwdt; xcnt < x + lwdt; ++xcnt)
			{
				DrawPix(sfcDest, xcnt, y + ycnt, Pattern.PatternClr(xcnt, y + ycnt));
			}
		}
		else
		{
			for (int xcnt = x - lwdt; xcnt < x + lwdt; ++xcnt)
			{
				sfcDest->SetPixDw(xcnt, y + ycnt, Pattern.PatternClr(xcnt, y + ycnt));
			}
		}
	}
	if (!fRenderTarget) sfcDest->Unlock();
}

void C4Draw::Grayscale(C4Surface * sfcSfc, int32_t iOffset)
{
	// safety
	if (!sfcSfc) return;
	// change colors
	int xcnt,ycnt,wdt=sfcSfc->Wdt,hgt=sfcSfc->Hgt;
	// Lock surface
	if (!sfcSfc->Lock()) return;
	for (ycnt=0; ycnt<hgt; ycnt++)
	{
		for (xcnt=0; xcnt<wdt; xcnt++)
		{
			DWORD dwColor = sfcSfc->GetPixDw(xcnt,ycnt,false);
			uint32_t r = GetRedValue(dwColor), g = GetGreenValue(dwColor), b = GetBlueValue(dwColor), a = dwColor >> 24;
			int32_t gray = Clamp<int32_t>((r + g + b) / 3 + iOffset, 0, 255);
			sfcSfc->SetPixDw(xcnt, ycnt, RGBA(gray, gray, gray, a));
		}
	}
	sfcSfc->Unlock();
}

bool C4Draw::GetPrimaryClipper(int &rX1, int &rY1, int &rX2, int &rY2)
{
	// Store drawing clip values
	rX1=fClipX1; rY1=fClipY1; rX2=fClipX2; rY2=fClipY2;
	// Done
	return true;
}

C4Rect C4Draw::GetClipRect() const
{
	int iWdt=Min(iClipX2, RenderTarget->Wdt-1)-iClipX1+1;
	int iHgt=Min(iClipY2, RenderTarget->Hgt-1)-iClipY1+1;
	int iX=iClipX1; if (iX<0) { iWdt+=iX; iX=0; }
	int iY=iClipY1; if (iY<0) { iHgt+=iY; iY=0; }
	return C4Rect(iX, iY, iWdt, iHgt);
}

C4Rect C4Draw::GetOutRect() const
{
	return C4Rect(0, 0, RenderTarget->Wdt, RenderTarget->Hgt);
}

void C4Draw::SetGamma(DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, int32_t iRampIndex)
{
	// No gamma effects
	if (Config.Graphics.DisableGamma) return;
	if (iRampIndex < 0 || iRampIndex >= C4MaxGammaRamps) return;
	// turn ramp index into array offset
	iRampIndex*=3;
	// set array members
	dwGamma[iRampIndex+0]=dwClr1;
	dwGamma[iRampIndex+1]=dwClr2;
	dwGamma[iRampIndex+2]=dwClr3;
	// mark gamma ramp to be recalculated
	fSetGamma=true;
}

void C4Draw::ResetGamma()
{
	pApp->ApplyGammaRamp(DefRamp.ramp, false);
}

void C4Draw::ApplyGamma()
{
	// No gamma effects
	if (Config.Graphics.DisableGamma) return;
	if (!fSetGamma) return;

	//  calculate color channels by adding the difference between the gamma ramps to their normals
	int32_t ChanOff[3];
	DWORD tGamma[3];
	const int32_t DefChanVal[3] = { 0x00, 0x80, 0xff };
	// calc offset for curve points
	for (int32_t iCurve=0; iCurve<3; ++iCurve)
	{
		memset(ChanOff, 0, sizeof(int32_t)*3);
		// ...channels...
		for (int32_t iChan=0; iChan<3; ++iChan)
			// ...ramps...
			for (int32_t iRamp=0; iRamp<C4MaxGammaRamps; ++iRamp)
				// add offset
				ChanOff[iChan]+=(int32_t) BYTE(dwGamma[iRamp*3+iCurve]>>(16-iChan*8)) - DefChanVal[iCurve];
		// calc curve point
		tGamma[iCurve]=C4RGB(Clamp<int32_t>(DefChanVal[iCurve]+ChanOff[0], 0, 255), Clamp<int32_t>(DefChanVal[iCurve]+ChanOff[1], 0, 255), Clamp<int32_t>(DefChanVal[iCurve]+ChanOff[2], 0, 255));
	}
	// calc ramp
	Gamma.Set(tGamma[0], tGamma[1], tGamma[2]);
	// set gamma
	pApp->ApplyGammaRamp(Gamma.ramp, false);
	fSetGamma=false;
}

void C4Draw::DisableGamma()
{
	// set it
	pApp->ApplyGammaRamp(DefRamp.ramp, true);
}

void C4Draw::EnableGamma()
{
	// set it
	pApp->ApplyGammaRamp(Gamma.ramp, false);
}

DWORD C4Draw::ApplyGammaTo(DWORD dwClr)
{
	return Gamma.ApplyTo(dwClr);
}

void C4Draw::SetZoom(float X, float Y, float Zoom)
{
	this->ZoomX = X; this->ZoomY = Y; this->Zoom = Zoom;
}

void C4Draw::ApplyZoom(float & X, float & Y)
{
	X = (X - ZoomX) * Zoom + ZoomX;
	Y = (Y - ZoomY) * Zoom + ZoomY;
}

void C4Draw::RemoveZoom(float & X, float & Y)
{
	X = (X - ZoomX) / Zoom + ZoomX;
	Y = (Y - ZoomY) / Zoom + ZoomY;
}

bool DDrawInit(C4AbstractApp * pApp, unsigned int iXRes, unsigned int iYRes, int iBitDepth, unsigned int iMonitor)
{
	// create engine
    #ifndef USE_CONSOLE
	  pDraw = new CStdGL();
    #else
	  pDraw = new CStdNoGfx();
    #endif
	if (!pDraw) return false;
	// init it
	if (!pDraw->Init(pApp, iXRes, iYRes, iBitDepth, iMonitor))
	{
		delete pDraw;
		return false;
	}
	// done, success
	return true;
}

bool C4Draw::Init(C4AbstractApp * pApp, unsigned int iXRes, unsigned int iYRes, int iBitDepth, unsigned int iMonitor)
{
	this->pApp = pApp;

	// store default gamma
	if (!pApp->SaveDefaultGammaRamp(DefRamp.ramp))
		DefRamp.Default();

	pApp->pWindow->pSurface = new C4Surface(pApp, pApp->pWindow);

	if (!CreatePrimarySurfaces(iXRes, iYRes, iBitDepth, iMonitor))
		return false;

	if (!CreatePrimaryClipper(iXRes, iYRes))
		return Error("  Clipper failure.");

	return true;
}

void C4Draw::DrawBoxFade(C4Surface * sfcDest, float iX, float iY, float iWdt, float iHgt, DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, DWORD dwClr4, int iBoxOffX, int iBoxOffY)
{
	// set vertex buffer data
	// vertex order:
	// 0=upper left   dwClr1
	// 1=lower left   dwClr3
	// 2=lower right  dwClr4
	// 3=upper right  dwClr2
	float vtx[8];
	vtx[0] = iX     ; vtx[1] = iY;
	vtx[2] = iX     ; vtx[3] = iY+iHgt;
	vtx[4] = iX+iWdt; vtx[5] = iY+iHgt;
	vtx[6] = iX+iWdt; vtx[7] = iY;
	DrawQuadDw(sfcDest, vtx, dwClr1, dwClr3, dwClr4, dwClr2);
}

void C4Draw::DrawBoxDw(C4Surface * sfcDest, int iX1, int iY1, int iX2, int iY2, DWORD dwClr)
{
	if (!sfcDest->IsRenderTarget())
	{
		// Box on non-render target: Emulate by setting pixels
		if (!sfcDest->Lock()) return;
		for (int y = iY1; y <= iY2; ++y)
			for (int x = iX1; x <= iX2; ++x)
				sfcDest->SetPixDw(x,y, dwClr);
		sfcDest->Unlock();
	}
	else
	{
		DrawBoxFade(sfcDest, float(iX1), float(iY1), float(iX2 - iX1 + 1), float(iY2 - iY1 + 1), dwClr, dwClr, dwClr, dwClr, 0, 0);
	}
}
