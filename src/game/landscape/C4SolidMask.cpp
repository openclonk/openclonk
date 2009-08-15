/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2003, 2005, 2008  Sven Eberhardt
 * Copyright (c) 2007  Peter Wortmann
 * Copyright (c) 2007-2009  Günther Brammer
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

/* Solid areas of objects, put into the landscape */

#include <C4Include.h>
#include <C4SolidMask.h>

#ifndef BIG_C4INCLUDE
#include <C4Object.h>
#include <C4Landscape.h>
#include <C4Game.h>
#include <C4GameObjects.h>
#endif


void C4SolidMask::Put(bool fCauseInstability, C4TargetRect *pClipRect, bool fRestoreAttachment)
	{
	// If not put, put mask to background,
	// storing background pixels in cSolidMask.

  // No mask
  if (!pSolidMask || !pSolidMaskMatBuff) { iAttachingObjectsCount = 0; return; }
  // Contained
  if (pForObject->Contained) { iAttachingObjectsCount = 0; return; }
  // Mask is put
	if (fCauseInstability) CheckConsistency();

	bool RegularPut;
	if (!pClipRect)
		{
		// Regular Put: Update MaskPutRect and MaskPutRotation
		MaskPutRotation = pForObject->r;
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
  // Lock mask surface
  int iPitch = pForObject->SolidMask.Wdt;
	int xcnt,ycnt,iTx,iTy;
	// Put mask pixels
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
			if (MaskPutRect.x < 0) { MaskPutRect.tx = -MaskPutRect.x; MaskPutRect.x = 0; } else MaskPutRect.tx = 0;
			MaskPutRect.y = oy;
			if (MaskPutRect.y < 0) { MaskPutRect.ty = -MaskPutRect.y; MaskPutRect.y = 0; } else MaskPutRect.ty = 0;
			MaskPutRect.Wdt = Min<int32_t>(ox + pForObject->SolidMask.Wdt, GBackWdt) - MaskPutRect.x;
			MaskPutRect.Hgt = Min<int32_t>(oy + pForObject->SolidMask.Hgt, GBackHgt) - MaskPutRect.y;
			}
		// fill rect with mask
		for (ycnt=0; ycnt<pClipRect->Hgt; ++ycnt)
			{
			BYTE *pPix=pSolidMask + (ycnt+pClipRect->ty)*pForObject->SolidMask.Wdt + pClipRect->tx;
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
						byPixel=GBackPix(iTx,iTy);
						// store it. If MCVehic, also store in initial put, but won't be used in restore
						// do not overwrite current value in re-put issued by SolidMask-remover
						if (byPixel != MCVehic || RegularPut)
							pSolidMaskMatBuff[(ycnt+pClipRect->ty)*MatBuffPitch+xcnt+pClipRect->tx]=byPixel;
						}
					// and set mask
					_SBackPix(iTx,iTy,MCVehic);
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
		FIXED Ma1 = Cos(itofix(-MaskPutRotation)), Ma2 = -Sin(itofix(-MaskPutRotation)),
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
			if (MaskPutRect.x < 0) { MaskPutRect.tx = -MaskPutRect.x; MaskPutRect.Wdt = MaskPutRect.x; MaskPutRect.x = 0; } else { MaskPutRect.tx = 0; MaskPutRect.Wdt = 0; }
			MaskPutRect.y = ystart;
			if (MaskPutRect.y < 0) { MaskPutRect.ty = -MaskPutRect.y; MaskPutRect.Hgt = MaskPutRect.y; MaskPutRect.y = 0; } else { MaskPutRect.ty = 0; MaskPutRect.Hgt = 0; }
			MaskPutRect.Wdt = Min<int32_t>(xstart + MatBuffPitch, GBackWdt) - MaskPutRect.x;
			MaskPutRect.Hgt = Min<int32_t>(ystart + MatBuffPitch, GBackHgt) - MaskPutRect.y;
			}
		// go through clipping rect
		FIXED yfoo = itofix(pClipRect->ty - MatBuffPitch/2);
		iTy=pClipRect->y;
		int w = pForObject->SolidMask.Wdt;
		int h = pForObject->SolidMask.Hgt;
		for(ycnt = 0; ycnt < pClipRect->Hgt; ycnt++)
			{
			FIXED yfooa = yfoo * Ma2;
			FIXED yfoob = yfoo * Mb2;
			C4Fixed xfoo = itofix(pClipRect->tx - MatBuffPitch/2);
			iTx=pClipRect->x;
			int i = (ycnt + pClipRect->ty) * MatBuffPitch + pClipRect->tx;
			for(xcnt = 0; xcnt < pClipRect->Wdt; xcnt++)
				{
				// calc position in solidmask buffer
				int iMx = fixtoi(xfoo * Ma1 + yfooa) + w / 2;
				int iMy = fixtoi(xfoo * Mb1 + yfoob) + h / 2;
				// in bounds? and solidmask?
				if(iMx >= 0 && iMy >= 0 && iMx < w && iMy < h && pSolidMask[iMy*iPitch+iMx])
					{
					// is background mat to be stored?
					if (!MaskPut)
						{
						// get background pixel
						byPixel=_GBackPix(iTx,iTy);
						// store it. If MCVehic, also store in initial put, but won't be used in restore
						// do not overwrite current value in re-put issued by SolidMask-remover
						if (byPixel != MCVehic || RegularPut)
							pSolidMaskMatBuff[i + xcnt] = byPixel;
						}
					// set mask pix
					_SBackPix(iTx, iTy, MCVehic);
					}
				else if (!MaskPut)
					// mark pix as unused in buf
					pSolidMaskMatBuff[i+xcnt] = MCVehic;
				xfoo += 1;
				++iTx;
				}
			yfoo += 1;
			++iTy;
			}
		}
	// Store mask put status
	MaskPut=true;
	// restore attached object positions if moved
	if (fRestoreAttachment && iAttachingObjectsCount)
		{
		int32_t dx = pForObject->GetX() - MaskRemovalX;
		int32_t dy = pForObject->GetY() - MaskRemovalY;
		if (dx|dy)
			for (int i = 0; i < iAttachingObjectsCount; ++i)
				{
				C4Object *pObj = ppAttachingObjects[i];
				if (pObj->IsMoveableBySolidMask())
					if (!pObj->Shape.ContactCheck(pObj->GetX()+dx, pObj->GetY()+dy))
						if (pObj->iLastAttachMovementFrame != Game.FrameCounter)
							{
							pObj->iLastAttachMovementFrame = Game.FrameCounter;
							pObj->MovePosition(dx, dy);
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
	BYTE *pPix;
	if (!rSolidMaskData.MaskPutRotation)
		{
		pPix=rSolidMaskData.pSolidMask+(y+rSolidMaskData.MaskPutRect.ty)*rSolidMaskData.pForObject->SolidMask.Wdt+rSolidMaskData.MaskPutRect.tx+x;
		if (*pPix == 0xff)
			return C4M_Solid;
		else
			return 0;
		}
	else
		{
		// Using put-buffer for rotated masks
		// for SolidMask-pixels not put because there was another SolidMask already, this will not return solid
		pPix=rSolidMaskData.pSolidMaskMatBuff+(y+rSolidMaskData.MaskPutRect.ty)*rSolidMaskData.MatBuffPitch+rSolidMaskData.MaskPutRect.tx+x;
		if (*pPix == MCVehic)
			return 0;
		else
			return C4M_Solid;
		}
	}

void C4SolidMask::Remove(bool fCauseInstability, bool fBackupAttachment)
	{
	// If put, restore background pixels from buffer

	// Not put
  if (!MaskPut || !pSolidMask || !pSolidMaskMatBuff) return;

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
				assert(_GBackPix(iTx,iTy) == MCVehic);
				_SBackPixIfMask(iTx,iTy,*pPix,MCVehic);
				// Instability
				if (fCauseInstability)
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
		MaskRemovalX = pForObject->GetX();
		MaskRemovalY = pForObject->GetY();
		iAttachingObjectsCount = 0;
		C4LArea SolidArea(&::Objects.Sectors, MaskPutRect.x-1, MaskPutRect.y-1, MaskPutRect.Wdt+2, MaskPutRect.Hgt+2);
		C4LSector *pSct; C4Object *pObj;
		for (C4ObjectList *pLst=SolidArea.FirstObjectShapes(&pSct); pLst; pLst=SolidArea.NextObjectShapes(pLst, &pSct))
			for (C4ObjectLink *clnk=pLst->First; clnk; clnk=clnk->Next)
				if ((pObj = clnk->Obj) && pObj != pForObject && pObj->IsMoveableBySolidMask() && !pObj->Shape.CheckContact(pObj->GetX(),pObj->GetY()))
					{
					// check for any contact to own SolidMask - attach-directions, bottom - "stuck" (CNAT_Center) is ignored, because that causes problems with things being stuck in basements :(
					int iVtx = 0;
					for (; iVtx < pObj->Shape.VtxNum; ++iVtx)
						if (pObj->Shape.GetVertexContact(iVtx, pObj->Action.t_attach | CNAT_Bottom, pObj->GetX(), pObj->GetY(), DensityProvider(pForObject, *this)))
							if (pObj->Shape.GetVertexContact(iVtx, pObj->Action.t_attach | CNAT_Bottom, pObj->GetX(), pObj->GetY(), DensityProvider(pForObject, *this)))
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

void C4SolidMask::Clear()
	{
	// free mask+mat-buffer
	if (pSolidMask) { delete [] pSolidMask; pSolidMask = NULL; }
	if (pSolidMaskMatBuff) { delete [] pSolidMaskMatBuff; pSolidMaskMatBuff = NULL; }
	// safety: mask cannot be removed now
	MaskPut = false;
	// clear attaching objects
	delete [] ppAttachingObjects; ppAttachingObjects = NULL;
	iAttachingObjectsCount = iAttachingObjectsCapacity = 0;
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
	if (!MaskPut || !pSolidMask || !pSolidMaskMatBuff) return;
	where.Intersect(MaskPutRect);
	// reput background pixels
	for (int y = where.y; y < where.y + where.Hgt; ++y)
		{
		for (int x = where.x; x < where.x + where.Wdt; ++x)
			{
			BYTE *pPix = pSolidMaskMatBuff + (y - MaskPutRect.y + MaskPutRect.ty) * MatBuffPitch + x - MaskPutRect.x + MaskPutRect.tx;
			// only if mask was used here
			if (*pPix != MCVehic)
				{
				// restore
				assert(GBackPix(x,y)==MCVehic);
				_SBackPix(x, y, *pPix);
				}
			}
		}
	}

void C4SolidMask::PutTemporary(C4Rect where)
	{
	if (!MaskPut || !pSolidMask || !pSolidMaskMatBuff) return;
	where.Intersect(MaskPutRect);
	// reput vehicle pixels
	for (int y = where.y; y < where.y + where.Hgt; ++y)
		{
		for (int x = where.x; x < where.x + where.Wdt; ++x)
			{
			BYTE *pPix = pSolidMaskMatBuff + (y - MaskPutRect.y + MaskPutRect.ty) * MatBuffPitch + x - MaskPutRect.x + MaskPutRect.tx;
			// only if mask was used here
			if (*pPix != MCVehic)
				{
				// put
				assert(GBackPix(x,y)==*pPix);
				_SBackPix(x,y,MCVehic);
				}
			}
		}
	}

void C4SolidMask::Repair(C4Rect where)
	{
	if (!MaskPut || !pSolidMask || !pSolidMaskMatBuff) return;
	where.Intersect(MaskPutRect);
	// reput vehicle pixels
	for (int y = where.y; y < where.y + where.Hgt; ++y)
		{
		for (int x = where.x; x < where.x + where.Wdt; ++x)
			{
			BYTE *pPix = pSolidMaskMatBuff + (y - MaskPutRect.y + MaskPutRect.ty) * MatBuffPitch + x - MaskPutRect.x + MaskPutRect.tx;
			// only if mask was used here
			if (*pPix != MCVehic)
				{
				// record changed landscape in MatBuff
				*pPix = GBackPix(x,y);
				// put
				_SBackPix(x,y,MCVehic);
				}
			}
		}
	}

C4SolidMask::C4SolidMask(C4Object *pForObject) : pForObject(pForObject)
	{
	// zero fields
	MaskPut=false;
	MaskPutRotation=0;
	MaskRemovalX=MaskRemovalY=0;
	ppAttachingObjects=NULL;
	iAttachingObjectsCount=iAttachingObjectsCapacity=0;
	// Update linked list
	Next = 0;
	Prev = Last;
	Last = this;
	if (Prev) Prev->Next = this;
	else First = this;
	// copy solid mask from bitmap
	int iNeededBufSize = pForObject->SolidMask.Wdt * pForObject->SolidMask.Hgt;
	if (!(pSolidMask = new BYTE [iNeededBufSize])) return;
	SURFACE sfcBitmap = pForObject->GetGraphics()->GetBitmap();
	if (!sfcBitmap->Lock()) return;
	int xcnt, ycnt;
	for(ycnt=0; ycnt<pForObject->SolidMask.Hgt; ycnt++)
		for(xcnt=0; xcnt<pForObject->SolidMask.Wdt; xcnt++)
			{
			// Solid mask target x/y is relative to def bitmap top-left, not object center.
			pSolidMask[xcnt+ycnt*pForObject->SolidMask.Wdt] = sfcBitmap->IsPixTransparent(pForObject->SolidMask.x+xcnt,pForObject->SolidMask.y+ycnt) ? 0x00 : 0xff;
			}
	// create mat buff to store the material replaced by the solid mask
	// the upper left corner is here the [objpos]+rot([shapexy]+[targetxy]+[realWH]/2)-maxWH/2
	MatBuffPitch = (int) sqrt(double(pForObject->SolidMask.Wdt * pForObject->SolidMask.Wdt + pForObject->SolidMask.Hgt * pForObject->SolidMask.Hgt))+1;
	if (!(pSolidMaskMatBuff= new BYTE [MatBuffPitch * MatBuffPitch] )) return;
	ZeroMemory(pSolidMaskMatBuff, MatBuffPitch * MatBuffPitch);
	sfcBitmap->Unlock();
	}

C4SolidMask::~C4SolidMask()
	{
	// Update linked list
	if (Next) Next->Prev = Prev;
	if (Prev) Prev->Next = Next;
	if (First == this) First = Next;
	if (Last == this) Last = Prev;
	// clear fields
	Clear();
	}

C4SolidMask * C4SolidMask::First = 0;
C4SolidMask * C4SolidMask::Last = 0;


#ifdef SOLIDMASK_DEBUG

bool C4SolidMask::CheckConsistency()
	{
	C4Rect SolidMaskRect(0,0,GBackWdt,GBackHgt);
	C4SolidMask *pSolid;
	for (pSolid = C4SolidMask::Last; pSolid; pSolid = pSolid->Prev)
		{
		pSolid->RemoveTemporary(SolidMaskRect);
		}
	assert(!::Landscape.MatCount[MVehic]);
	// Restore Solidmasks
	for (pSolid = C4SolidMask::First; pSolid; pSolid = pSolid->Next)
		{
		pSolid->PutTemporary(SolidMaskRect);
		}
	return true;
	}

#endif
