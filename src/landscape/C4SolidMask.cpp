/*
 * OpenClonk, http://www.openclonk.org
 *
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

/* Solid areas of objects, put into the landscape */

#include "C4Include.h"
#include "landscape/C4SolidMask.h"

#include "graphics/C4DrawGL.h"
#include "graphics/CSurface8.h"
#include "graphics/StdPNG.h"
#include "landscape/C4Landscape.h"
#include "landscape/C4Material.h"
#include "object/C4Def.h"
#include "object/C4GameObjects.h"
#include "object/C4Object.h"


void C4SolidMask::Put(bool fCauseInstability, C4TargetRect *pClipRect, bool fRestoreAttachment)
{
	// If not put, put mask to background,
	// storing background pixels in cSolidMask.

	// No mask
	if (!pForObject || !pForObject->Def || !pForObject->Def->pSolidMask || !pSolidMaskMatBuff) { iAttachingObjectsCount = 0; return; }
	// Contained
	if (pForObject->Contained) { iAttachingObjectsCount = 0; return; }
	// Mask is put
	if (fCauseInstability) CheckConsistency();

	bool RegularPut;
	if (!pClipRect)
	{
		// Regular Put: Update MaskPutRect and MaskPutRotation
		MaskPutRotation = pForObject->GetR();
		pClipRect = &MaskPutRect;
		RegularPut = true;
	}
	else
	{
		// Reput by C4SolidMask::Remove
		// Don't change MaskPutRotation or MaskPutRect
		// Intersect ClipRect with the MaskPutRect
		if (!pClipRect->ClipBy(MaskPutRect)) return;
		RegularPut = false;
	}
	// Get mask surface
	CSurface8 *pSolidMask = pForObject->Def->pSolidMask;
	// Put mask pixels
	int xcnt,ycnt,iTx,iTy;
	BYTE byPixel;
	// not rotated?
	if (!MaskPutRotation)
	{
		// calc put rect
		if (RegularPut)
		{
			int ox, oy;
			ox = pForObject->GetX() + pForObject->Def->Shape.x + pForObject->SolidMask.tx;
			oy = pForObject->GetY() + pForObject->Def->Shape.y + pForObject->SolidMask.ty;
			MaskPutRect.x = ox;
			if (MaskPutRect.x < 0) { MaskPutRect.tx = -MaskPutRect.x; MaskPutRect.x = 0; }
			else MaskPutRect.tx = 0;
			MaskPutRect.y = oy;
			if (MaskPutRect.y < 0) { MaskPutRect.ty = -MaskPutRect.y; MaskPutRect.y = 0; }
			else MaskPutRect.ty = 0;
			MaskPutRect.Wdt = std::min<int32_t>(ox + pForObject->SolidMask.Wdt, ::Landscape.GetWidth()) - MaskPutRect.x;
			MaskPutRect.Hgt = std::min<int32_t>(oy + pForObject->SolidMask.Hgt, ::Landscape.GetHeight()) - MaskPutRect.y;
		}
		// fill rect with mask
		for (ycnt=0; ycnt<pClipRect->Hgt; ++ycnt)
		{
			BYTE *pPix=pSolidMask->Bits + (ycnt+pClipRect->ty+pForObject->SolidMask.y)*pSolidMask->Pitch + pClipRect->tx + pForObject->SolidMask.x;
			for (xcnt=0; xcnt<pClipRect->Wdt; ++xcnt,++pPix)
			{
				if (*pPix)
				{
					// solid mask present here
					// calc position in landscape
					iTx=pClipRect->x+xcnt; iTy=pClipRect->y+ycnt;
					// is background mat to be stored? always do this in the given rect
					if (!MaskPut)
					{
						// get background pixel
						byPixel=::Landscape.GetPix(iTx,iTy);
						// store it. If MCVehic, also store in initial put, but won't be used in restore
						// do not overwrite current value in re-put issued by SolidMask-remover
						if (!IsSomeVehicle(byPixel) || RegularPut)
							pSolidMaskMatBuff[(ycnt+pClipRect->ty)*MatBuffPitch+xcnt+pClipRect->tx]=byPixel;
					}
					// and set mask
					::Landscape.SetPix2(iTx,iTy,MaskMaterial,::Landscape.Transparent);
				}
				else
					// no SolidMask: mark buffer as unused here
					if (!MaskPut)
						pSolidMaskMatBuff[(ycnt+pClipRect->ty)*MatBuffPitch+xcnt+pClipRect->tx]=MCVehic;
			}
		}
	}
	else
	{
		// calc matrix for given rotation
		C4Real Ma1 = Cos(itofix(-MaskPutRotation)), Ma2 = -Sin(itofix(-MaskPutRotation)),
		            Mb1 = Sin(itofix(-MaskPutRotation)), Mb2 = Cos(itofix(-MaskPutRotation));
		// get upper-left corner of landscape copy rect
		int centerx = pForObject->Def->Shape.x + pForObject->SolidMask.tx + pForObject->SolidMask.Wdt / 2;
		int centery = pForObject->Def->Shape.y + pForObject->SolidMask.ty + pForObject->SolidMask.Hgt / 2;
		int xstart = pForObject->GetX() + fixtoi(Ma1 * itofix(centerx) - Ma2 * itofix(centery)) - MatBuffPitch / 2;
		int ystart = pForObject->GetY() + fixtoi(-Mb1 * itofix(centerx) + Mb2 * itofix(centery)) - MatBuffPitch / 2;
		// store put rect
		if (RegularPut)
		{
			MaskPutRect.x = xstart;
			if (MaskPutRect.x < 0) { MaskPutRect.tx = -MaskPutRect.x; MaskPutRect.Wdt = MaskPutRect.x; MaskPutRect.x = 0; }
			else { MaskPutRect.tx = 0; MaskPutRect.Wdt = 0; }
			MaskPutRect.y = ystart;
			if (MaskPutRect.y < 0) { MaskPutRect.ty = -MaskPutRect.y; MaskPutRect.Hgt = MaskPutRect.y; MaskPutRect.y = 0; }
			else { MaskPutRect.ty = 0; MaskPutRect.Hgt = 0; }
			MaskPutRect.Wdt = std::min<int32_t>(xstart + MatBuffPitch, ::Landscape.GetWidth()) - MaskPutRect.x;
			MaskPutRect.Hgt = std::min<int32_t>(ystart + MatBuffPitch, ::Landscape.GetHeight()) - MaskPutRect.y;
		}
		// go through clipping rect
		const C4Real y0 = itofix(pClipRect->ty - MatBuffPitch/2);
		const C4Real x0 = itofix(pClipRect->tx - MatBuffPitch/2);
		iTy=pClipRect->y;
		int32_t w = pForObject->SolidMask.Wdt;
		int32_t h = pForObject->SolidMask.Hgt;
		int32_t mx0 = pForObject->SolidMask.x;
		int32_t my0 = pForObject->SolidMask.y;
		C4Real ya = y0 * Ma2;
		C4Real yb = y0 * Mb2;
		for (ycnt = 0; ycnt < pClipRect->Hgt; ycnt++)
		{
			iTx=pClipRect->x;
			int i = (ycnt + pClipRect->ty) * MatBuffPitch + pClipRect->tx;
			C4Real xa = x0 * Ma1;
			C4Real xb = x0 * Mb1;
			for (xcnt = 0; xcnt < pClipRect->Wdt; xcnt++)
			{
				// calc position in solidmask buffer
				int32_t iMx = fixtoi(xa + ya) + w / 2;
				int32_t iMy = fixtoi(xb + yb) + h / 2;
				// in bounds? and solidmask?
				if (iMx >= 0 && iMy >= 0 && iMx < w && iMy < h && pSolidMask->_GetPix(iMx+mx0, iMy+my0))
				{
					// is background mat to be stored?
					if (!MaskPut)
					{
						// get background pixel
						byPixel=::Landscape._GetPix(iTx,iTy);
						// store it. If MCVehic, also store in initial put, but won't be used in restore
						// do not overwrite current value in re-put issued by SolidMask-remover
						if (!IsSomeVehicle(byPixel) || RegularPut)
							pSolidMaskMatBuff[i + xcnt] = byPixel;
					}
					// set mask pix
					::Landscape.SetPix2(iTx, iTy, MaskMaterial, ::Landscape.Transparent);
				}
				else if (!MaskPut)
					// mark pix as unused in buf
					pSolidMaskMatBuff[i+xcnt] = MCVehic;
				xa += Ma1; xb += Mb1;
				++iTx;
			}
			ya += Ma2; yb += Mb2;
			++iTy;
		}
	}
	// Store mask put status
	MaskPut=true;
	// restore attached object positions if moved
	if (fRestoreAttachment && iAttachingObjectsCount)
	{
		C4Real dx = pForObject->GetFixedX() - MaskRemovalX;
		int32_t dy = pForObject->GetY() - MaskRemovalY;
		if (dx != Fix0 || dy != 0)
			for (int i = 0; i < iAttachingObjectsCount; ++i)
			{
				C4Object *pObj = ppAttachingObjects[i];
				if (pObj->IsMoveableBySolidMask(pForObject->GetSolidMaskPlane()))
					if (!pObj->Shape.ContactCheck(fixtoi(pObj->GetFixedX()+dx), fixtoi(pObj->GetFixedY()+dy)))
						if (pObj->iLastAttachMovementFrame != Game.FrameCounter)
						{
							pObj->iLastAttachMovementFrame = Game.FrameCounter;
							pObj->MovePosition(dx, itofix(dy));
						}
			}
		iAttachingObjectsCount = 0;
	}

	if (fCauseInstability) CheckConsistency();
}

int32_t C4SolidMask::DensityProvider::GetDensity(int32_t x, int32_t y) const
{
	// outside SolidMask: free
	x -= rSolidMaskData.MaskPutRect.x;
	y -= rSolidMaskData.MaskPutRect.y;
	if (!Inside<int32_t>(x, 0, rSolidMaskData.MaskPutRect.Wdt-1)
	    || !Inside<int32_t>(y, 0, rSolidMaskData.MaskPutRect.Hgt-1))
		return 0;
	// check put mask. Easy for unrotated
	BYTE pix;
	if (!rSolidMaskData.MaskPutRotation)
	{
		CSurface8 *pSolidMask = rSolidMaskData.pForObject->Def->pSolidMask;
		if (!pSolidMask) return 0; // can't really happen
		pix=pSolidMask->_GetPix(x+rSolidMaskData.pForObject->SolidMask.x+rSolidMaskData.MaskPutRect.tx,
		                        y+rSolidMaskData.pForObject->SolidMask.y+rSolidMaskData.MaskPutRect.ty);
		if (pix == 0xff)
			return C4M_Vehicle;
		else
			return 0;
	}
	else
	{
		// Using put-buffer for rotated masks
		// for SolidMask-pixels not put because there was another SolidMask already, this will not return solid
		pix=*(rSolidMaskData.pSolidMaskMatBuff+(y+rSolidMaskData.MaskPutRect.ty)*rSolidMaskData.MatBuffPitch+rSolidMaskData.MaskPutRect.tx+x);
		if (IsSomeVehicle(pix))
			return 0;
		else
			return C4M_Vehicle;
	}
}

void C4SolidMask::Remove(bool fBackupAttachment)
{
	// If put, restore background pixels from buffer

	// Not put
	if (!MaskPut || !pSolidMaskMatBuff) return;

	CheckConsistency();

	// reput background pixels
	for (int ycnt=0; ycnt<MaskPutRect.Hgt; ++ycnt)
	{
		BYTE *pPix=pSolidMaskMatBuff+(ycnt+MaskPutRect.ty)*MatBuffPitch+MaskPutRect.tx;
		for (int xcnt=0; xcnt<MaskPutRect.Wdt; ++xcnt,++pPix)
			// only if mask was used here
			if (*pPix != MCVehic)
			{
				// calc position in landscape
				int iTx=MaskPutRect.x+xcnt; int iTy=MaskPutRect.y+ycnt;
				// restore pixel here
				// The pPix-check ensures that only pixels that hads been overwritten by this SolidMask are restored
				// Non-SolidMask-pixels should not happen here, because all relevant landscape change routines should
				// temp remove SolidMasks before
				assert(IsSomeVehicle(::Landscape._GetPix(iTx,iTy)));
				if (IsSomeVehicle(::Landscape._GetPix(iTx, iTy)))
					::Landscape._SetPix2(iTx, iTy, *pPix, ::Landscape.Transparent);
				// Instability
				::Landscape.CheckInstabilityRange(iTx,iTy);
			}
	}
	// Mask not put flag
	MaskPut=false;
	// update surrounding masks in that range
	C4TargetRect ClipRect;
	for (C4SolidMask *pSolid = C4SolidMask::Last; pSolid; pSolid = pSolid->Prev)
		if (pSolid->MaskPut) if (pSolid->MaskPutRect.Overlap(MaskPutRect))
			{
				// set clipping rect for all calls, since they may modify it
				ClipRect.Set(MaskPutRect.x, MaskPutRect.y, MaskPutRect.Wdt, MaskPutRect.Hgt, 0, 0);
				// doubled solidmask-pixels have just been removed in the clipped area!
				pSolid->MaskPut = false;
				// re-put the solidmask
				pSolid->Put(false, &ClipRect, false);
			}

	// backup attachment if desired: Backup old pos and all objects that attach to or lie on the SolidMask
	if (fBackupAttachment)
	{
		MaskRemovalX = pForObject->GetFixedX();
		MaskRemovalY = pForObject->GetY();
		iAttachingObjectsCount = 0;
		// Search in area slightly larger than SolidMask because objects might have vertices slightly outside their shape
		C4LArea SolidArea(&::Objects.Sectors, MaskPutRect.x-1, MaskPutRect.y-4, MaskPutRect.Wdt+2, MaskPutRect.Hgt+2);
		C4LSector *pSct;
		for (C4ObjectList *pLst=SolidArea.FirstObjectShapes(&pSct); pLst; pLst=SolidArea.NextObjectShapes(pLst, &pSct))
			for (C4Object *pObj : *pLst)
				if (pObj && pObj != pForObject && pObj->IsMoveableBySolidMask(pForObject->GetSolidMaskPlane()) && !pObj->Shape.CheckContact(pObj->GetX(),pObj->GetY()))
				{
					// avoid duplicate that may be found due to sector overlaps
					bool has_dup = false;
					for (int32_t i_dup = 0; i_dup < iAttachingObjectsCount; ++i_dup)
						if (ppAttachingObjects[i_dup] == pObj)
						{
							has_dup = true;
							break;
						}
					if (has_dup) continue;
					// check for any contact to own SolidMask - attach-directions, bottom - "stuck" (CNAT_Center) is ignored, because that causes problems with things being stuck in basements :(
					int iVtx = 0;
					for (; iVtx < pObj->Shape.VtxNum; ++iVtx)
						if (pObj->Shape.GetVertexContact(iVtx, pObj->Action.t_attach | CNAT_Bottom, pObj->GetX(), pObj->GetY(), DensityProvider(*this)))
							break;
					if (iVtx == pObj->Shape.VtxNum) continue; // no contact
					// contact: Add object to list
					if (iAttachingObjectsCapacity == iAttachingObjectsCount)
					{
						iAttachingObjectsCapacity += 4;
						C4Object **ppNewAttachingObjects = new C4Object *[iAttachingObjectsCapacity];
						if (iAttachingObjectsCount) memcpy(ppNewAttachingObjects, ppAttachingObjects, sizeof(C4Object *) * iAttachingObjectsCount);
						delete [] ppAttachingObjects;
						ppAttachingObjects = ppNewAttachingObjects;
					}
					ppAttachingObjects[iAttachingObjectsCount++] = pObj;
				}
	}

	CheckConsistency();
}

void C4SolidMask::Draw(C4TargetFacet &cgo)
{
	// only if put
	if (!MaskPut) return;
	// set topface facet
	C4Facet fct; fct.Set(pForObject->GetGraphics()->GetBitmap(), pForObject->SolidMask.x, pForObject->SolidMask.y, pForObject->SolidMask.Wdt, pForObject->SolidMask.Hgt);
	// draw it
	if (MaskPutRotation)
		fct.DrawXR(cgo.Surface, pForObject->GetX()+pForObject->Shape.x+cgo.X-cgo.TargetX+pForObject->SolidMask.tx, pForObject->GetY()+pForObject->Shape.y+cgo.Y-cgo.TargetY+pForObject->SolidMask.ty, fct.Wdt, fct.Hgt, 0, 0, MaskPutRotation);
	else
		fct.DrawX(cgo.Surface, pForObject->GetX()+pForObject->Shape.x+cgo.X-cgo.TargetX+pForObject->SolidMask.tx, pForObject->GetY()+pForObject->Shape.y+cgo.Y-cgo.TargetY+pForObject->SolidMask.ty, fct.Wdt, fct.Hgt, 0, 0);
}


void C4SolidMask::RemoveTemporary(C4Rect where)
{
	if (!MaskPut || !pSolidMaskMatBuff) return;
	where.Intersect(MaskPutRect);
	// reput background pixels
	for (int y = where.y; y < where.y + where.Hgt; ++y)
	{
		for (int x = where.x; x < where.x + where.Wdt; ++x)
		{
			BYTE *pPix = pSolidMaskMatBuff + (y - MaskPutRect.y + MaskPutRect.ty) * MatBuffPitch + x - MaskPutRect.x + MaskPutRect.tx;
			// only if mask was used here
			if (!IsSomeVehicle(*pPix)) //
			{
				// restore
				assert(IsSomeVehicle(::Landscape.GetPix(x,y)));
				::Landscape._SetPix2Tmp(x, y, *pPix, ::Landscape.Transparent);
			}
		}
	}
}

void C4SolidMask::PutTemporary(C4Rect where)
{
	if (!MaskPut || !pSolidMaskMatBuff) return;
	where.Intersect(MaskPutRect);
	// reput vehicle pixels
	for (int y = where.y; y < where.y + where.Hgt; ++y)
	{
		for (int x = where.x; x < where.x + where.Wdt; ++x)
		{
			BYTE *pPix = pSolidMaskMatBuff + (y - MaskPutRect.y + MaskPutRect.ty) * MatBuffPitch + x - MaskPutRect.x + MaskPutRect.tx;
			// only if mask was used here
			if (!IsSomeVehicle(*pPix))
			{
				// put
				assert(::Landscape.GetPix(x,y)==*pPix);
				::Landscape._SetPix2Tmp(x, y, MaskMaterial, ::Landscape.Transparent);
			}
		}
	}
}

void C4SolidMask::Repair(C4Rect where)
{
	if (!MaskPut || !pSolidMaskMatBuff) return;
	where.Intersect(MaskPutRect);
	// reput vehicle pixels
	for (int y = where.y; y < where.y + where.Hgt; ++y)
	{
		for (int x = where.x; x < where.x + where.Wdt; ++x)
		{
			BYTE *pPix = pSolidMaskMatBuff + (y - MaskPutRect.y + MaskPutRect.ty) * MatBuffPitch + x - MaskPutRect.x + MaskPutRect.tx;
			// only if mask was used here
			if (!IsSomeVehicle(*pPix))
			{
				// record changed landscape in MatBuff
				*pPix = ::Landscape.GetPix(x,y);
				// put
				::Landscape.SetPix2(x, y, MaskMaterial, ::Landscape.Transparent);
			}
		}
	}
}

C4SolidMask::C4SolidMask(C4Object *pForObject) : pForObject(pForObject)
{
	// zero fields
	MaskPut=false;
	MaskPutRotation=0;
	MaskRemovalX = Fix0;
	MaskRemovalY = 0;
	ppAttachingObjects=nullptr;
	iAttachingObjectsCount=iAttachingObjectsCapacity=0;
	MaskMaterial=MCVehic;
	// Update linked list
	Next = nullptr;
	Prev = Last;
	Last = this;
	if (Prev) Prev->Next = this;
	else First = this;
	// create mat buff to store the material replaced by the solid mask
	// the upper left corner is here the [objpos]+rot([shapexy]+[targetxy]+[realWH]/2)-maxWH/2
	MatBuffPitch = (int) sqrt(double(pForObject->SolidMask.Wdt * pForObject->SolidMask.Wdt + pForObject->SolidMask.Hgt * pForObject->SolidMask.Hgt))+1;
	if (!(pSolidMaskMatBuff= new BYTE [MatBuffPitch * MatBuffPitch] )) return;
	memset(pSolidMaskMatBuff, 0, MatBuffPitch * MatBuffPitch);
}

C4SolidMask::~C4SolidMask()
{
	Remove(false);
	// Update linked list
	if (Next) Next->Prev = Prev;
	if (Prev) Prev->Next = Next;
	if (First == this) First = Next;
	if (Last == this) Last = Prev;
	delete [] pSolidMaskMatBuff;
	delete [] ppAttachingObjects;
}

void C4SolidMask::RemoveSolidMasks()
{
	C4Rect SolidMaskRect(0,0,::Landscape.GetWidth(),::Landscape.GetHeight());
	C4SolidMask *pSolid;
	for (pSolid = C4SolidMask::Last; pSolid; pSolid = pSolid->Prev)
	{
		pSolid->RemoveTemporary(SolidMaskRect);
	}
}

void C4SolidMask::PutSolidMasks()
{
	C4Rect SolidMaskRect(0,0,::Landscape.GetWidth(),::Landscape.GetHeight());
	C4SolidMask *pSolid;
	// Restore Solidmasks
	for (pSolid = C4SolidMask::First; pSolid; pSolid = pSolid->Next)
	{
		pSolid->PutTemporary(SolidMaskRect);
	}
}

C4SolidMask * C4SolidMask::First = nullptr;
C4SolidMask * C4SolidMask::Last = nullptr;


bool C4SolidMask::CheckConsistency()
{
	if (!SOLIDMASK_DEBUG)
		return true;

	C4Rect SolidMaskRect(0,0,::Landscape.GetWidth(),::Landscape.GetHeight());
	C4SolidMask *pSolid;
	for (pSolid = C4SolidMask::Last; pSolid; pSolid = pSolid->Prev)
	{
		pSolid->RemoveTemporary(SolidMaskRect);
	}
	assert(!::Landscape.GetMatCount(MVehic));
	// Restore Solidmasks
	for (pSolid = C4SolidMask::First; pSolid; pSolid = pSolid->Next)
	{
		pSolid->PutTemporary(SolidMaskRect);
	}
	return true;
}

CSurface8 *C4SolidMask::LoadMaskFromFile(class C4Group &hGroup, const char *szFilename)
{
	// Construct SolidMask surface from PNG bitmap:
	// All pixels that are more than 50% transparent are not solid
	CPNGFile png;
	StdBuf png_buf;
	if (!hGroup.LoadEntry(szFilename, &png_buf)) return nullptr; // error messages done by caller
	if (!png.Load((BYTE*)png_buf.getMData(), png_buf.getSize())) return nullptr;
	CSurface8 *result = new CSurface8(png.iWdt, png.iHgt);
	for (size_t y=0u; y<png.iHgt; ++y)
		for (size_t x=0u; x<png.iWdt; ++x)
			result->SetPix(x,y,((png.GetPix(x,y)>>24)<128) ? 0x00 : 0xff);
	return result;
}

void C4SolidMask::SetHalfVehicle(bool set)
{
	MaskMaterial = set ? MCHalfVehic : MCVehic;
	// TODO: Redraw
}
