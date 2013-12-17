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

/* Pixel Sprite system for tiny bits of moving material */

#include <C4Include.h>
#include <C4PXS.h>

#include <C4Components.h>
#include <C4Config.h>
#include <C4Physics.h>
#include <C4Random.h>
#include <C4Weather.h>
#include <C4Game.h>
#include <C4Record.h>

static const C4Real WindDrift_Factor = itofix(1, 800);

void C4PXS::Execute()
{
#ifdef DEBUGREC_PXS
	if (Config.General.DebugRec)
	{
		C4RCExecPXS rc;
		rc.x=x; rc.y=y; rc.iMat=Mat;
		rc.pos = 0;
		AddDbgRec(RCT_ExecPXS, &rc, sizeof(rc));
	}
#endif
	int32_t inmat;

	// Safety
	if (!MatValid(Mat))
		{ Deactivate(); return; }

	// Out of bounds
	if ((x<0) || (x>=GBackWdt) || (y<-10) || (y>=GBackHgt))
		{ Deactivate(); return; }

	// Material conversion
	int32_t iX = fixtoi(x), iY = fixtoi(y);
	inmat=GBackMat(iX,iY);
	C4MaterialReaction *pReact = ::MaterialMap.GetReactionUnsafe(Mat, inmat);
	if (pReact && (*pReact->pFunc)(pReact, iX,iY, iX,iY, xdir,ydir, Mat,inmat, meePXSPos, NULL))
		{ Deactivate(); return; }

	// Gravity
	ydir+=GravAccel;

	if (GBackDensity(iX, iY + 1) < ::MaterialMap.Map[Mat].Density)
	{
		// Air speed: Wind plus some random
		int32_t iWind = GBackWind(iX, iY);
		C4Real txdir = itofix(iWind, 15) + C4REAL256(Random(1200) - 600);
		C4Real tydir = C4REAL256(Random(1200) - 600);

		// Air friction, based on WindDrift. MaxSpeed is ignored.
		int32_t iWindDrift = Max(::MaterialMap.Map[Mat].WindDrift - 20, 0);
		xdir += ((txdir - xdir) * iWindDrift) * WindDrift_Factor;
		ydir += ((tydir - ydir) * iWindDrift) * WindDrift_Factor;
	}

	C4Real ctcox = x + xdir;
	C4Real ctcoy = y + ydir;

	int32_t iToX = fixtoi(ctcox), iToY = fixtoi(ctcoy);

	// In bounds?
	if (Inside<int32_t>(iToX, 0, GBackWdt-1) && Inside<int32_t>(iToY, 0, GBackHgt-1))
		// Check path
		if (::Landscape._PathFree(iX, iY, iToX, iToY))
		{
			x=ctcox; y=ctcoy;
			return;
		}

	// Test path to target position
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
				return;
			}
			else
			{
				// no destructive contact, but speed or position changed: Stop moving for now
				if (fStopMovement)
				{
					x = itofix(iX); y = itofix(iY);
					return;
				}
				// there was a reaction func, but it didn't do anything - continue movement
			}
		}
		iX = inX; iY = inY;
	}
	while (iX != iToX || iY != iToY);

	// No contact? Free movement
	x=ctcox; y=ctcoy;
#ifdef DEBUGREC_PXS
	if (Config.General.DebugRec)
	{
		C4RCExecPXS rc;
		rc.x=x; rc.y=y; rc.iMat=Mat;
		rc.pos = 1;
		AddDbgRec(RCT_ExecPXS, &rc, sizeof(rc));
	}
#endif
	return;
}

void C4PXS::Deactivate()
{
#ifdef DEBUGREC_PXS
	if (Config.General.DebugRec)
	{
		C4RCExecPXS rc;
		rc.x=x; rc.y=y; rc.iMat=Mat;
		rc.pos = 2;
		AddDbgRec(RCT_ExecPXS, &rc, sizeof(rc));
	}
#endif
	Mat=MNone;
	::PXS.Delete(this);
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
	for (unsigned int cnt=0; cnt<PXSMaxChunk; cnt++)
	{
		Chunk[cnt]=NULL;
		iChunkPXS[cnt]=0;
	}
}

void C4PXSSystem::Clear()
{
	for (unsigned int cnt=0; cnt<PXSMaxChunk; cnt++)
	{
		if (Chunk[cnt]) delete [] Chunk[cnt];
		Chunk[cnt]=NULL;
		iChunkPXS[cnt]=0;
	}
}

C4PXS* C4PXSSystem::New()
{
	unsigned int cnt,cnt2;
	C4PXS *pxp;
	// Check chunks for available space
	for (cnt=0; cnt<PXSMaxChunk; cnt++)
	{
		// Create new chunk if necessary
		if (!Chunk[cnt])
		{
			if (!(Chunk[cnt]=new C4PXS[PXSChunkSize])) return NULL;
			iChunkPXS[cnt] = 0;
		}
		// Check this chunk for space
		if (iChunkPXS[cnt] < PXSChunkSize)
			for (cnt2=0,pxp=Chunk[cnt]; cnt2<PXSChunkSize; cnt2++,pxp++)
				if (pxp->Mat==MNone)
				{
					// count theam
					iChunkPXS[cnt]++;
					return pxp;
				}
	}
	return NULL;
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
	// Execute all chunks
	Count=0;
	for (unsigned int cchunk=0; cchunk<PXSMaxChunk; cchunk++)
		if (Chunk[cchunk])
		{
			// empty chunk?
			if (!iChunkPXS[cchunk])
				{ delete [] Chunk[cchunk]; Chunk[cchunk]=NULL; }
			else
			{
				// Execute chunk pxs, check for empty
				C4PXS *pxp = Chunk[cchunk];
				for (unsigned int cnt2=0; cnt2<PXSChunkSize; cnt2++,pxp++)
					if (pxp->Mat!=MNone)
					{
						pxp->Execute();
						Count++;
					}
			}
		}
}

void C4PXSSystem::Draw(C4TargetFacet &cgo)
{

	// Draw PXS in this region
	C4Rect VisibleRect(cgo.TargetX, cgo.TargetY, cgo.Wdt, cgo.Hgt);
	VisibleRect.Enlarge(20);

	// First pass: draw simple PXS (lines/pixels)
	float cgox = cgo.X - cgo.TargetX, cgoy = cgo.Y - cgo.TargetY;
	unsigned int cnt;
	for (cnt=0; cnt < PXSMaxChunk; cnt++)
		if (Chunk[cnt] && iChunkPXS[cnt])
		{
			C4PXS *pxp = Chunk[cnt];
			for (unsigned int cnt2 = 0; cnt2 < PXSChunkSize; cnt2++, pxp++)
				if (pxp->Mat != MNone && VisibleRect.Contains(fixtoi(pxp->x), fixtoi(pxp->y)))
				{
					C4Material *pMat=&::MaterialMap.Map[pxp->Mat];
					if (pMat->PXSFace.Surface && Config.Graphics.PXSGfx)
						continue;
					// old-style: unicolored pixels or lines
					DWORD dwMatClr = ::Landscape.GetPal()->GetClr((BYTE) (Mat2PixColDefault(pxp->Mat)));
					if (fixtoi(pxp->xdir) || fixtoi(pxp->ydir))
					{
						// lines for stuff that goes whooosh!
						int len = fixtoi(Abs(pxp->xdir) + Abs(pxp->ydir));
						dwMatClr = uint32_t(Max<int>(dwMatClr >> 24, 195 - (195 - (dwMatClr >> 24)) / len)) << 24 | (dwMatClr & 0xffffff);
						pDraw->DrawLineDw(cgo.Surface,
						                              fixtof(pxp->x - pxp->xdir) + cgox, fixtof(pxp->y - pxp->ydir) + cgoy,
						                              fixtof(pxp->x) + cgox, fixtof(pxp->y) + cgoy,
						                              dwMatClr);
					}
					else
						// single pixels for slow stuff
						pDraw->DrawPix(cgo.Surface, fixtof(pxp->x) + cgox, fixtof(pxp->y) + cgoy, dwMatClr);
				}
		}

	// PXS graphics disabled?
	if (!Config.Graphics.PXSGfx)
		return;

	// Second pass: draw sprites
	for (cnt=0; cnt < PXSMaxChunk; cnt++)
		if (Chunk[cnt] && iChunkPXS[cnt])
		{
			C4PXS *pxp = Chunk[cnt];
			for (unsigned int cnt2 = 0; cnt2 < PXSChunkSize; cnt2++, pxp++)
				if (pxp->Mat != MNone && VisibleRect.Contains(fixtoi(pxp->x), fixtoi(pxp->y)))
				{
					C4Material * pMat = &::MaterialMap.Map[pxp->Mat];
					if (!pMat->PXSFace.Surface)
						continue;
					int32_t pnx, pny;
					pMat->PXSFace.GetPhaseNum(pnx, pny);
					int32_t fcWdt = pMat->PXSFace.Wdt; //int32_t fcWdtH = Max(fcWdt / 3, 1);
					// calculate draw width and tile to use (random-ish)
					uint32_t size = (1103515245 * (cnt * PXSChunkSize + cnt2) + 12345) >> 3;
					float z = pMat->PXSGfxSize * (0.625f + 0.05f * int(size % 16));
					pny = (cnt2 / pnx) % pny; pnx = cnt2 % pnx;
					// draw
					pMat->PXSFace.DrawX(cgo.Surface,
					                    fixtof(pxp->x) + cgox + z * pMat->PXSGfxRt.tx / fcWdt, fixtof(pxp->y) + cgoy + z * pMat->PXSGfxRt.ty / fcWdt,
					                    z, z * pMat->PXSFace.Hgt / fcWdt, pnx, pny);
				}
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
	unsigned int cnt;

	//Log("Save PXS");

	// Check used chunk count
	int32_t iChunks=0;
	for (cnt=0; cnt<PXSMaxChunk; cnt++)
		if (Chunk[cnt] && iChunkPXS[cnt])
			iChunks++;
	if (!iChunks)
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
	for (cnt=0; cnt<PXSMaxChunk; cnt++)
		if (Chunk[cnt]) // must save all chunks in order to keep order consistent on all clients
			if (!hTempFile.Write(Chunk[cnt],PXSChunkSize * sizeof(C4PXS)))
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
	size_t iBinSize,iChunkNum,cnt2;
	size_t iChunkSize = PXSChunkSize * sizeof(C4PXS);
	if (!hGroup.AccessEntry(C4CFN_PXS,&iBinSize)) return false;
	// clear previous
	Clear();
	// using C4Real or float?
	int32_t iNumForm = 1;
	if (iBinSize % iChunkSize == 4)
	{
		if (!hGroup.Read(&iNumForm, sizeof (iNumForm))) return false;
		if (!Inside<int32_t>(iNumForm, 1, 2)) return false;
		iBinSize -= 4;
	}
	// old pxs-files have no tag for the number format
	else if (iBinSize % iChunkSize != 0) return false;
	// calc chunk count
	iChunkNum = iBinSize / iChunkSize;
	if (iChunkNum > PXSMaxChunk) return false;
	for (uint32_t cnt=0; cnt<iChunkNum; cnt++)
	{
		if (!(Chunk[cnt]=new C4PXS[PXSChunkSize])) return false;
		if (!hGroup.Read(Chunk[cnt],iChunkSize)) return false;
		// count the PXS, Peter!
		// convert num format, if neccessary
		C4PXS *pxp; iChunkPXS[cnt]=0;
		for (cnt2=0,pxp=Chunk[cnt]; cnt2<PXSChunkSize; cnt2++,pxp++)
			if (pxp->Mat != MNone)
			{
				++iChunkPXS[cnt];
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

void C4PXSSystem::Synchronize()
{
	Count=0;
}

void C4PXSSystem::SyncClearance()
{
	// consolidate chunks; remove empty chunks
	C4PXS **pDestChunk = Chunk;
	int32_t iDestChunk = 0;
	for (unsigned int cnt=0; cnt<PXSMaxChunk; cnt++)
	{
		if (Chunk[cnt])
		{
			if (iChunkPXS[cnt])
			{
				*pDestChunk++ = Chunk[cnt];
				iChunkPXS[iDestChunk++] = iChunkPXS[cnt];
			}
			else
			{
				delete [] Chunk[cnt];
				Chunk[cnt] = NULL;
			}
		}
	}
}

void C4PXSSystem::Delete(C4PXS *pPXS)
{
	// find chunk
	unsigned int cnt;
	for (cnt = 0; cnt < PXSMaxChunk; cnt++)
		if (Chunk[cnt] && iChunkPXS[cnt])
			if (pPXS >= Chunk[cnt] && pPXS < Chunk[cnt] + PXSChunkSize)
				break;
	// decrease pxs counter
	if (cnt < PXSMaxChunk)
		iChunkPXS[cnt]--;
}

C4PXSSystem PXS;
