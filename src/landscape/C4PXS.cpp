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

/* Pixel Sprite system for tiny bits of moving material */

#include "C4Include.h"
#include "landscape/C4PXS.h"

#include "c4group/C4Components.h"
#include "control/C4Record.h"
#include "game/C4Physics.h"
#include "graphics/C4Draw.h"
#include "landscape/C4Weather.h"
#include "lib/C4Random.h"
#include "lib/StdColors.h"

static const C4Real WindDrift_Factor = itofix(1, 800);

bool C4PXS::Execute()
{
	if (DEBUGREC_PXS && Config.General.DebugRec)
	{
		C4RCExecPXS rc;
		rc.x=x; rc.y=y; rc.iMat=Mat;
		rc.pos = 0;
		AddDbgRec(RCT_ExecPXS, &rc, sizeof(rc));
	}
	int32_t inmat;

	// Safety
	if (!MatValid(Mat))
		{ Deactivate(); return false; }

	// Out of bounds
	if ((x<0) || (x>=::Landscape.GetWidth()) || (y<-10) || (y>=::Landscape.GetHeight()))
		{ Deactivate(); return false; }

	// Material conversion
	int32_t iX = fixtoi(x), iY = fixtoi(y);
	inmat=GBackMat(iX,iY);
	C4MaterialReaction *pReact = ::MaterialMap.GetReactionUnsafe(Mat, inmat);
	if (pReact && (*pReact->pFunc)(pReact, iX,iY, iX,iY, xdir,ydir, Mat,inmat, meePXSPos, nullptr))
		{ Deactivate(); return false; }

	// Gravity
	ydir+=GravAccel;

	if (GBackDensity(iX, iY + 1) < ::MaterialMap.Map[Mat].Density)
	{
		// Air speed: Wind plus some random
		int32_t iWind = Weather.GetWind(iX, iY);
		C4Real txdir = itofix(iWind, 15) + C4REAL256(Random(1200) - 600);
		C4Real tydir = C4REAL256(Random(1200) - 600);

		// Air friction, based on WindDrift. MaxSpeed is ignored.
		int32_t iWindDrift = std::max(::MaterialMap.Map[Mat].WindDrift - 20, 0);
		xdir += ((txdir - xdir) * iWindDrift) * WindDrift_Factor;
		ydir += ((tydir - ydir) * iWindDrift) * WindDrift_Factor;
	}

	C4Real ctcox = x + xdir;
	C4Real ctcoy = y + ydir;

	int32_t iToX = fixtoi(ctcox), iToY = fixtoi(ctcoy);

	// In bounds?
	if (Inside<int32_t>(iToX, 0, ::Landscape.GetWidth()-1) && Inside<int32_t>(iToY, 0, ::Landscape.GetHeight()-1))
		// Check path
		if (::Landscape._PathFree(iX, iY, iToX, iToY))
		{
			x=ctcox; y=ctcoy;
			return true;
		}

	// Test path to target position
	int32_t iX0 = iX, iY0 = iY;
	bool fStopMovement = false;
	do
	{
		// Step
		int32_t inX = iX + Sign(iToX - iX), inY = iY + Sign(iToY - iY);
		// Contact?
		inmat = GBackMat(inX, inY);
		C4MaterialReaction *pReact = ::MaterialMap.GetReactionUnsafe(Mat, inmat);
		if (pReact)
		{
			if ((*pReact->pFunc)(pReact, iX,iY, inX,inY, xdir,ydir, Mat,inmat, meePXSMove, &fStopMovement))
			{
				// destructive contact
				Deactivate();
				return false;
			}
			else
			{
				// no destructive contact, but speed or position changed: Stop moving for now
				if (fStopMovement)
				{
					// But keep fractional positions to allow proper movement on moving ground
					if (iX != iX0) x = itofix(iX);
					if (iY != iY0) y = itofix(iY);
					return true;
				}
				// there was a reaction func, but it didn't do anything - continue movement
			}
		}
		iX = inX; iY = inY;
	}
	while (iX != iToX || iY != iToY);

	// No contact? Free movement
	x=ctcox; y=ctcoy;
	if (DEBUGREC_PXS && Config.General.DebugRec)
	{
		C4RCExecPXS rc;
		rc.x=x; rc.y=y; rc.iMat=Mat;
		rc.pos = 1;
		AddDbgRec(RCT_ExecPXS, &rc, sizeof(rc));
	}
	return true;
}

void C4PXS::Deactivate()
{
	if (DEBUGREC_PXS && Config.General.DebugRec)
	{
		C4RCExecPXS rc;
		rc.x=x; rc.y=y; rc.iMat=Mat;
		rc.pos = 2;
		AddDbgRec(RCT_ExecPXS, &rc, sizeof(rc));
	}
	Mat=MNone;
}

C4PXSSystem::C4PXSSystem()
{
	Default();
}

C4PXSSystem::~C4PXSSystem()
{
	Clear();
}

void C4PXSSystem::Default()
{
	Count=0;
}

void C4PXSSystem::Clear()
{
	Count=0;
}

C4PXS* C4PXSSystem::New()
{
	if (Count < PXSMax)
		return &PXS[Count++];
	return nullptr;
}

bool C4PXSSystem::Create(int32_t mat, C4Real ix, C4Real iy, C4Real ixdir, C4Real iydir)
{
	C4PXS *pxp;
	if (!MatValid(mat)) return false;
	if (!(pxp=New())) return false;
	pxp->Mat=mat;
	pxp->x=ix; pxp->y=iy;
	pxp->xdir=ixdir; pxp->ydir=iydir;
	return true;
}

void C4PXSSystem::Execute()
{
	for (size_t i = 0; i < Count; i++)
	{
		if (!PXS[i].Execute())
		{
			assert(PXS[i].Mat == MNone);
			PXS[i--] = PXS[--Count];
		}
	}
}

void C4PXSSystem::Draw(C4TargetFacet &cgo)
{
	// Draw PXS in this region
	C4Rect VisibleRect(cgo.TargetX, cgo.TargetY, cgo.Wdt, cgo.Hgt);
	VisibleRect.Enlarge(20);

	// Go through all PXS and build vertex arrays. The vertex arrays are
	// then submitted to the GL in one go.
	std::vector<C4BltVertex> pixVtx;
	std::vector<C4BltVertex> lineVtx;
	std::map<int, std::vector<C4BltVertex> > bltVtx;
	// TODO: reserve some space to avoid too many allocations
	// TODO: keep vertex mem allocated between draw invocations

	float cgox = cgo.X - cgo.TargetX, cgoy = cgo.Y - cgo.TargetY;
	// First pass: draw simple PXS (lines/pixels)
	for (size_t i = 0; i < Count; i++)
	{
		C4PXS *pxp = &PXS[i];
		if (pxp->Mat != MNone && VisibleRect.Contains(fixtoi(pxp->x), fixtoi(pxp->y)))
		{
			C4Material *pMat = &::MaterialMap.Map[pxp->Mat];
			const DWORD dwMatClr = ::Landscape.GetPal()->GetClr((BYTE) (Mat2PixColDefault(pxp->Mat)));
			if(pMat->PXSFace.Surface)
			{
				int32_t pnx, pny;
				pMat->PXSFace.GetPhaseNum(pnx, pny);
				int32_t fcWdt = pMat->PXSFace.Wdt;
				int32_t fcHgt = pMat->PXSFace.Hgt;
				// calculate draw width and tile to use (random-ish)
				uint32_t size = (1103515245 * i + 12345) >> 3;
				float z = pMat->PXSGfxSize * (0.625f + 0.05f * int(size % 16));
				pny = (i / pnx) % pny; pnx = i % pnx;

				const float w = z;
				const float h = z * fcHgt / fcWdt;
				const float x1 = fixtof(pxp->x) + cgox + z * pMat->PXSGfxRt.tx / fcWdt;
				const float y1 = fixtof(pxp->y) + cgoy + z * pMat->PXSGfxRt.ty / fcHgt;
				const float x2 = x1 + w;
				const float y2 = y1 + h;

				const float sfcWdt = pMat->PXSFace.Surface->Wdt;
				const float sfcHgt = pMat->PXSFace.Surface->Hgt;

				C4BltVertex vtx[6];
				vtx[0].tx = (pnx + 0.f) * fcWdt / sfcWdt; vtx[0].ty = (pny + 0.f) * fcHgt / sfcHgt;
				vtx[0].ftx = x1; vtx[0].fty = y1;
				vtx[1].tx = (pnx + 1.f) * fcWdt / sfcWdt; vtx[1].ty = (pny + 0.f) * fcHgt / sfcHgt;
				vtx[1].ftx = x2; vtx[1].fty = y1;
				vtx[2].tx = (pnx + 1.f) * fcWdt / sfcWdt; vtx[2].ty = (pny + 1.f) * fcHgt / sfcHgt;
				vtx[2].ftx = x2; vtx[2].fty = y2;
				vtx[3].tx = (pnx + 0.f) * fcWdt / sfcWdt; vtx[3].ty = (pny + 1.f) * fcHgt / sfcHgt;
				vtx[3].ftx = x1; vtx[3].fty = y2;
				DwTo4UB(0xFFFFFFFF, vtx[0].color);
				DwTo4UB(0xFFFFFFFF, vtx[1].color);
				DwTo4UB(0xFFFFFFFF, vtx[2].color);
				DwTo4UB(0xFFFFFFFF, vtx[3].color);
				vtx[4] = vtx[2];
				vtx[5] = vtx[0];

				std::vector<C4BltVertex>& vec = bltVtx[pxp->Mat];
				vec.push_back(vtx[0]);
				vec.push_back(vtx[1]);
				vec.push_back(vtx[2]);
				vec.push_back(vtx[3]);
				vec.push_back(vtx[4]);
				vec.push_back(vtx[5]);
			}
			else
			{
				// old-style: unicolored pixels or lines
				if (fixtoi(pxp->xdir) || fixtoi(pxp->ydir))
				{
					// lines for stuff that goes whooosh!
					int len = fixtoi(Abs(pxp->xdir) + Abs(pxp->ydir));
					const DWORD dwMatClrLen = uint32_t(std::max<int>(dwMatClr >> 24, 195 - (195 - (dwMatClr >> 24)) / len)) << 24 | (dwMatClr & 0xffffff);
					C4BltVertex begin, end;
					begin.ftx = fixtof(pxp->x - pxp->xdir) + cgox; begin.fty = fixtof(pxp->y - pxp->ydir) + cgoy;
					end.ftx = fixtof(pxp->x) + cgox; end.fty = fixtof(pxp->y) + cgoy;
					DwTo4UB(dwMatClrLen, begin.color);
					DwTo4UB(dwMatClrLen, end.color);
					lineVtx.push_back(begin);
					lineVtx.push_back(end);
				}
				else
				{
					// single pixels for slow stuff
					C4BltVertex vtx;
					vtx.ftx = fixtof(pxp->x) + cgox;
					vtx.fty = fixtof(pxp->y) + cgoy;
					DwTo4UB(dwMatClr, vtx.color);
					pixVtx.push_back(vtx);
				}
			}
		}
	}

	if(!pixVtx.empty()) pDraw->PerformMultiPix(cgo.Surface, &pixVtx[0], pixVtx.size(), nullptr);
	if(!lineVtx.empty()) pDraw->PerformMultiLines(cgo.Surface, &lineVtx[0], lineVtx.size(), 1.0f, nullptr);

	// PXS graphics disabled?
	if (!Config.Graphics.PXSGfx)
		return;

	for(std::map<int, std::vector<C4BltVertex> >::const_iterator iter = bltVtx.begin(); iter != bltVtx.end(); ++iter)
	{
		C4Material *pMat = &::MaterialMap.Map[iter->first];
		pDraw->PerformMultiTris(cgo.Surface, &iter->second[0], iter->second.size(), nullptr, pMat->PXSFace.Surface->texture.get(), nullptr, nullptr, 0, nullptr);
	}
}

void C4PXSSystem::Cast(int32_t mat, int32_t num, int32_t tx, int32_t ty, int32_t level)
{
	int32_t cnt;
	for (cnt=0; cnt<num; cnt++)
	{
		// Must do these calculation steps separately, because the order of
		// invokations of Random() is not defined if they're used as parameters
		C4Real xdir = itofix(Random(level+1)-level/2); xdir/=10;
		C4Real ydir = itofix(Random(level+1)-level); ydir/=10;
		Create(mat,
		       itofix(tx),itofix(ty),
		       xdir,
		       ydir);
	}
}

bool C4PXSSystem::Save(C4Group &hGroup)
{
	if (Count == 0)
	{
		hGroup.Delete(C4CFN_PXS);
		return true;
	}

	// Save chunks to temp file
	CStdFile hTempFile;
	if (!hTempFile.Create(Config.AtTempPath(C4CFN_TempPXS)))
		return false;
#ifdef C4REAL_USE_FIXNUM
	int32_t iNumFormat = 1;
#else
	int32_t iNumFormat = 2;
#endif
	if (!hTempFile.Write(&iNumFormat, sizeof (iNumFormat)))
		return false;
	if (!hTempFile.Write(PXS, Count * sizeof(C4PXS)))
		return false;

	if (!hTempFile.Close())
		return false;

	// Move temp file to group
	if (!hGroup.Move( Config.AtTempPath(C4CFN_TempPXS),
	                  C4CFN_PXS ))
		return false;

	return true;
}

bool C4PXSSystem::Load(C4Group &hGroup)
{
	// load new
	size_t iBinSize, PXSNum;
	if (!hGroup.AccessEntry(C4CFN_PXS,&iBinSize)) return false;
	// clear previous
	Clear();
	// using C4Real or float?
	int32_t iNumForm = 1;
	if (iBinSize % sizeof(C4PXS) == 4)
	{
		if (!hGroup.Read(&iNumForm, sizeof (iNumForm))) return false;
		if (!Inside<int32_t>(iNumForm, 1, 2)) return false;
		iBinSize -= 4;
	}
	// old pxs-files have no tag for the number format
	else if (iBinSize % sizeof(C4PXS) != 0) return false;
	// calc chunk count
	PXSNum = iBinSize / sizeof(C4PXS);
	if (PXSNum > PXSMax) return false;
	if (!hGroup.Read(PXS, iBinSize)) return false;
	// count the PXS, Peter!
	Count = PXSNum;
	// convert num format, if neccessary
	for (size_t i = 0; i < Count; i++)
	{
		C4PXS *pxp = &PXS[i];
		if (pxp->Mat != MNone)
		{
			// convert number format
#ifdef C4REAL_USE_FIXNUM
			if (iNumForm == 2) { FLOAT_TO_FIXED(&pxp->x); FLOAT_TO_FIXED(&pxp->y); FLOAT_TO_FIXED(&pxp->xdir); FLOAT_TO_FIXED(&pxp->ydir); }
#else
			if (iNumForm == 1) { FIXED_TO_FLOAT(&pxp->x); FIXED_TO_FLOAT(&pxp->y); FIXED_TO_FLOAT(&pxp->xdir); FIXED_TO_FLOAT(&pxp->ydir); }
#endif
		}
	}
	return true;
}

int32_t C4PXSSystem::GetCount(int32_t mat) const
{
	// count PXS of given material
	int32_t result = 0;
	for (size_t i = 0; i < Count; i++)
	{
		if (PXS[i].Mat == mat) ++result;
	}
	return result;
}

int32_t C4PXSSystem::GetCount(int32_t mat, int32_t x, int32_t y, int32_t wdt, int32_t hgt) const
{
	// count PXS of given material in given area
	int32_t result = 0;
	for (size_t i = 0; i < Count; i++)
	{
		const C4PXS *pxp = &PXS[i];
		if (pxp->Mat == mat || mat == MNone)
			if (Inside(pxp->x, x, x + wdt - 1) && Inside(pxp->y, y, y + hgt - 1))
				++result;
	}
	return result;
}

C4PXSSystem PXS;
