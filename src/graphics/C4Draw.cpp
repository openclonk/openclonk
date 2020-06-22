/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
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
#include "C4ForbidLibraryCompilation.h"
#include "graphics/C4Draw.h"

#include "graphics/C4DrawGL.h"
#include "graphics/C4DrawT.h"
#include "graphics/C4FontLoader.h"
#include "graphics/CSurface8.h"
#include "lib/C4Markup.h"
#include "lib/C4Rect.h"
#include "lib/StdColors.h"
#include "lib/StdMesh.h"
#include "platform/C4App.h"
#include "platform/C4Window.h"

// Instruct Optimus laptops to use nVidia GPU instead of integrated GPU
#if defined(_WIN32) && !defined(USE_CONSOLE)
extern "C" {
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
#endif

// Global access pointer
C4Draw *pDraw=nullptr;

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
		CachedPattern = nullptr;
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
	sfcPattern32=nullptr;
	CachedPattern = nullptr;
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
		sfcPattern32=nullptr;
	}
	delete[] CachedPattern; CachedPattern = nullptr;
}

DWORD C4Pattern::PatternClr(unsigned int iX, unsigned int iY) const
{
	if (!CachedPattern) return 0;
	// wrap position
	iX %= Wdt; iY %= Hgt;
	return CachedPattern[iY * Wdt + iX];
}

//--------------------------------------------------------------------

void C4Draw::Default()
{
	RenderTarget=nullptr;
	ClipAll=false;
	Active=false;
	BlitModulated=false;
	dwBlitMode = 0;
	ResetGamma();
	pFoW = nullptr;
	ZoomX = 0; ZoomY = 0; Zoom = 1;
	MeshTransform = nullptr;
	fUsePerspective = false;
	scriptUniform.Clear();
}

void C4Draw::Clear()
{
	ResetGamma();
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
	SetPrimaryClipper(std::max(iX1,iClipX1),std::max(iY1,iClipY1),std::min(iX2,iClipX2),std::min(iY2,iClipY2));
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
	return true;
}

bool C4Draw::DetachPrimaryClipper(C4Surface * sfcSurface)
{
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
				PerformMultiPix(sfcTarget, vertices, BUF_SIZE, nullptr);
				bufcnt = 0;
			}
		}

	}
	if(bufcnt > 0)
		PerformMultiPix(sfcTarget, vertices, bufcnt, nullptr);
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

	// bound
	if (ClipAll) return true;
	// inside screen?
	if (twdt<=0 || thgt<=0) return false;
	// prepare rendering to surface
	if (!PrepareRendering(sfcTarget)) return false;
	// texture present?
	if (!sfcSource->texture)
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
	if (sfcSource->pMainSfc) if (sfcSource->pMainSfc->texture) fBaseSfc = true;

	C4TexRef *pTex = sfcSource->texture.get();
	// set up blit data
	C4BltVertex vertices[6];
	vertices[0].ftx = tx; vertices[0].fty = ty;
	vertices[1].ftx = tx + twdt; vertices[1].fty = ty;
	vertices[2].ftx = tx + twdt; vertices[2].fty = ty + thgt;
	vertices[3].ftx = tx; vertices[3].fty = ty + thgt;
	vertices[0].tx = fx / pTex->iSizeX; vertices[0].ty = fy / pTex->iSizeY;
	vertices[1].tx = (fx + fwdt) / pTex->iSizeX; vertices[1].ty = fy / pTex->iSizeY;
	vertices[2].tx = (fx + fwdt) / pTex->iSizeX; vertices[2].ty = (fy + fhgt) / pTex->iSizeY;
	vertices[3].tx = fx / pTex->iSizeX; vertices[3].ty = (fy + fhgt) / pTex->iSizeY;
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
		pBaseTex = sfcSource->pMainSfc->texture.get();
	}

	C4TexRef* pNormalTex = nullptr;
	if (sfcSource->pNormalSfc)
		pNormalTex = sfcSource->pNormalSfc->texture.get();

	// ClrByOwner is always fully opaque
	const DWORD dwOverlayClrMod = 0xff000000 | sfcSource->ClrByOwnerClr;
	PerformMultiTris(sfcTarget, vertices, 6, pTransform, pBaseTex, fBaseSfc ? pTex : nullptr, pNormalTex, dwOverlayClrMod, nullptr);
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
	StdMeshMatrix mat = StdMeshMatrix::Identity();
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
	auto ttx0=(float)tx, tty0=(float)ty, ttx1=(float)(tx+twdt), tty1=(float)(ty+thgt);
	auto ttx2=(float)ttx0, tty2=(float)tty1, ttx3=(float)ttx1, tty3=(float)tty0;
	pTransform->TransformPoint(ttx0, tty0);
	pTransform->TransformPoint(ttx1, tty1);
	pTransform->TransformPoint(ttx2, tty2);
	pTransform->TransformPoint(ttx3, tty3);
	int ttxMin = std::max<int>((int)floor(std::min(std::min(ttx0, ttx1), std::min(ttx2, ttx3))), 0);
	int ttxMax = std::min<int>((int)ceil(std::max(std::max(ttx0, ttx1), std::max(ttx2, ttx3))), sfcTarget->Wdt);
	int ttyMin = std::max<int>((int)floor(std::min(std::min(tty0, tty1), std::min(tty2, tty3))), 0);
	int ttyMax = std::min<int>((int)ceil(std::max(std::max(tty0, tty1), std::max(tty2, tty3))), sfcTarget->Hgt);
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
		sfcSurface->pMainSfc = nullptr;
		Blit(sfcSurface, 0.0f, 0.0f, (float)sfcSurface->Wdt, (float)sfcSurface->Hgt, sfcTarget, float(tx), float(ty), float(sfcSurface->Wdt), float(sfcSurface->Hgt), false);
		sfcSurface->pMainSfc = pSfcBase;
		return true;
	}
}

bool C4Draw::BlitSurfaceTile(C4Surface * sfcSurface, C4Surface * sfcTarget, float iToX, float iToY, float iToWdt, float iToHgt, float iOffsetX, float iOffsetY, C4ShaderCall* shader_call)
{
	// Only direct rendering from single, tileable, texture
	if (!sfcTarget->IsRenderTarget()) return false;
	if ((sfcSurface->texture->iFlags & C4SF_Tileable) == 0) return false;

	// source surface dimensions
	const float sourceWdt = sfcSurface->Wdt;
	const float sourceHgt = sfcSurface->Hgt;

	// vertex positions
	C4BltVertex vertices[6];
	vertices[0].ftx = iToX; vertices[0].fty = iToY; vertices[0].ftz = 0.0f;
	vertices[0].tx = (0.0f + iOffsetX) / sourceWdt; vertices[0].ty = (0.0f + iOffsetY) / sourceHgt;
	DwTo4UB(0xffffffff, vertices[0].color);
	vertices[1].ftx = iToX + iToWdt; vertices[1].fty = iToY; vertices[1].ftz = 0.0f;
	vertices[1].tx = (iToWdt + iOffsetX) / sourceWdt; vertices[1].ty = (0.0f + iOffsetY) / sourceHgt;
	DwTo4UB(0xffffffff, vertices[1].color);
	vertices[2].ftx = iToX + iToWdt; vertices[2].fty = iToY + iToHgt; vertices[2].ftz = 0.0f;
	vertices[2].tx = (iToWdt + iOffsetX) / sourceWdt; vertices[2].ty = (iToHgt + iOffsetY) / sourceHgt;
	DwTo4UB(0xffffffff, vertices[2].color);
	vertices[3].ftx = iToX; vertices[3].fty = iToY + iToHgt; vertices[3].ftz = 0.0f;
	vertices[3].tx = (0.0f + iOffsetX) / sourceWdt; vertices[3].ty = (iToHgt + iOffsetY) / sourceHgt;
	DwTo4UB(0xffffffff, vertices[3].color);
	// duplicate vertices
	vertices[4] = vertices[0]; vertices[5] = vertices[2];

	// Draw
	PerformMultiTris(sfcTarget, vertices, 6, nullptr, sfcSurface->texture.get(), nullptr, nullptr, 0, shader_call);
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
	PerformMultiPix(sfcDest, &vtx, 1, nullptr);
}

void C4Draw::DrawLineDw(C4Surface * sfcTarget, float x1, float y1, float x2, float y2, DWORD dwClr, float width)
{
	C4BltVertex vertices[2];
	vertices[0].ftx = x1; vertices[0].fty = y1;
	vertices[1].ftx = x2; vertices[1].fty = y2;
	DwTo4UB(dwClr, vertices[0].color);
	DwTo4UB(dwClr, vertices[1].color);
	PerformMultiLines(sfcTarget, vertices, 2, width, nullptr);
}

void C4Draw::DrawCircleDw(C4Surface * sfcTarget, float cx, float cy, float r, DWORD dwClr, float width)
{
	// Draw as line segments
	int32_t num_lines = 12 + int32_t(r / 10);
	std::unique_ptr<C4BltVertex[]> vertices(new C4BltVertex[num_lines * 2]);
	for (int32_t i = 0; i < num_lines; ++i)
	{
		float ang = float(i) * 2 * M_PI / num_lines;
		int32_t iv = i * 2 + 1;
		vertices[iv].ftx = cx + sin(ang) * r;
		vertices[iv].fty = cy + cos(ang) * r;
		DwTo4UB(dwClr, vertices[iv].color);
		vertices[(iv + 1) % (num_lines * 2)] = vertices[iv];
	}
	PerformMultiLines(sfcTarget, vertices.get(), num_lines * 2, width, nullptr);
}

void C4Draw::DrawFrameDw(C4Surface * sfcDest, int x1, int y1, int x2, int y2, DWORD dwClr, float width) // make these parameters float...?
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

	for(auto & vertex : vertices)
		DwTo4UB(dwClr, vertex.color);

	PerformMultiLines(sfcDest, vertices, 8, width, nullptr);
}

void C4Draw::DrawQuadDw(C4Surface * sfcTarget, float *ipVtx, DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, DWORD dwClr4, C4ShaderCall* shader_call)
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
	PerformMultiTris(sfcTarget, vertices, 6, nullptr, nullptr, nullptr, nullptr, 0, shader_call);
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
	int iWdt=std::min(iClipX2, RenderTarget->Wdt-1)-iClipX1+1;
	int iHgt=std::min(iClipY2, RenderTarget->Hgt-1)-iClipY1+1;
	int iX=iClipX1; if (iX<0) { iWdt+=iX; iX=0; }
	int iY=iClipY1; if (iY<0) { iHgt+=iY; iY=0; }
	return C4Rect(iX, iY, iWdt, iHgt);
}

C4Rect C4Draw::GetOutRect() const
{
	return C4Rect(0, 0, RenderTarget->Wdt, RenderTarget->Hgt);
}

void C4Draw::SetGamma(float r, float g, float b, int32_t iRampIndex)
{
	// Set
	gamma[iRampIndex][0] = r;
	gamma[iRampIndex][1] = g;
	gamma[iRampIndex][2] = b;
	// Recalculate resulting gamma. Note that we flip gamma direction here,
	// because higher gammaOut means darker.
	gammaOut[0] = gammaOut[1] = gammaOut[2] = 1.0f;
	for (auto & i : gamma) {
		gammaOut[0] /= i[0];
		gammaOut[1] /= i[1];
		gammaOut[2] /= i[2];
	}
}

void C4Draw::ResetGamma()
{
	for (auto & i : gamma) {
		i[0] = 1.0f;
		i[1] = 1.0f;
		i[2] = 1.0f;
	}
	gammaOut[0] = 1.0f;
	gammaOut[1] = 1.0f;
	gammaOut[2] = 1.0f;
}

DWORD C4Draw::ApplyGammaTo(DWORD dwClr)
{
	return C4RGB(int(pow(float(GetRedValue(dwClr)) / 255.0f, gammaOut[0]) * 255.0),
				 int(pow(float(GetGreenValue(dwClr)) / 255.0f, gammaOut[1]) * 255.0),
				 int(pow(float(GetBlueValue(dwClr)) / 255.0f, gammaOut[2]) * 255.0));
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

bool DDrawInit(C4AbstractApp * pApp, unsigned int iXRes, unsigned int iYRes, unsigned int iMonitor)
{
	// create engine
    #ifndef USE_CONSOLE
	  pDraw = new CStdGL();
    #else
	  pDraw = new CStdNoGfx();
    #endif
	if (!pDraw) return false;
	// init it
	if (!pDraw->Init(pApp, iXRes, iYRes, iMonitor))
	{
		delete pDraw;
		return false;
	}
	// done, success
	return true;
}

bool C4Draw::Init(C4AbstractApp * pApp, unsigned int iXRes, unsigned int iYRes, unsigned int iMonitor)
{
	this->pApp = pApp;

	pApp->pWindow->pSurface = new C4Surface(pApp, pApp->pWindow);

	if (!RestoreDeviceObjects())
		return false;

	if (!CreatePrimaryClipper(iXRes, iYRes))
		return Error("  Clipper failure.");

	return true;
}

void C4Draw::DrawBoxFade(C4Surface * sfcDest, float iX, float iY, float iWdt, float iHgt, DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, DWORD dwClr4, C4ShaderCall* shader_call)
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
	DrawQuadDw(sfcDest, vtx, dwClr1, dwClr3, dwClr4, dwClr2, shader_call);
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
		DrawBoxFade(sfcDest, float(iX1), float(iY1), float(iX2 - iX1 + 1), float(iY2 - iY1 + 1), dwClr, dwClr, dwClr, dwClr, nullptr);
	}
}
