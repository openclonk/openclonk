/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2005, 2007  Matthes Bender
 * Copyright (c) 2001-2007  Sven Eberhardt
 * Copyright (c) 2003, 2005-2007  Peter Wortmann
 * Copyright (c) 2006-2009  Günther Brammer
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

/* Basic classes for rectangles and vertex outlines */

#include "C4Include.h"
#include "C4Rect.h"
#include "C4FacetEx.h"
#include "StdCompiler.h"
#include "StdAdaptors.h"

void C4Rect::Default()
{
	x=y=Wdt=Hgt=0;
}

void C4Rect::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkDefaultAdapt(x, 0)); pComp->Seperator();
	pComp->Value(mkDefaultAdapt(y, 0)); pComp->Seperator();
	pComp->Value(mkDefaultAdapt(Wdt, 0)); pComp->Seperator();
	pComp->Value(mkDefaultAdapt(Hgt, 0));
}

void C4TargetRect::Default()
{
	C4Rect::Default();
	tx=ty=0;
}

void C4TargetRect::Set(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, int32_t iTX, int32_t iTY)
{
	C4Rect::Set(iX,iY,iWdt,iHgt);
	tx=iTX; ty=iTY;
}

bool C4TargetRect::ClipBy(C4TargetRect &rClip)
{
	int32_t d;
	// clip left
	if ((d = x - rClip.x) < 0) { Wdt += d; x = rClip.x; }
	else tx += d;
	// clip top
	if ((d = y - rClip.y) < 0) { Hgt += d; y = rClip.y; }
	else ty += d;
	// clip right
	if ((d = (x+Wdt - rClip.x-rClip.Wdt)) > 0) Wdt -= d;
	// clip bottom
	if ((d = (y+Hgt - rClip.y-rClip.Hgt)) > 0) Hgt -= d;
	// check validity
	if (Wdt <= 0 || Hgt <= 0) return false;
	// add target pos
	tx += rClip.tx;
	ty += rClip.ty;
	// done
	return true;
}

void C4TargetRect::Set(const C4TargetFacet &rSrc)
{
	// copy members
	x=rSrc.X; y=rSrc.Y; Wdt=rSrc.Wdt; Hgt=rSrc.Hgt; tx=rSrc.TargetX; ty=rSrc.TargetY;
}

void C4TargetRect::CompileFunc(StdCompiler *pComp)
{
	C4Rect::CompileFunc(pComp); pComp->Seperator();
	pComp->Value(mkDefaultAdapt(tx,0)); pComp->Seperator();
	pComp->Value(mkDefaultAdapt(ty,0));
}

void C4Rect::Set(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt)
{
	x=iX; y=iY; Wdt=iWdt; Hgt=iHgt;
}

bool C4Rect::Overlap(C4Rect &rTarget)
{
	if (x+Wdt<=rTarget.x) return false;
	if (x>=rTarget.x+rTarget.Wdt) return false;
	if (y+Hgt<=rTarget.y) return false;
	if (y>=rTarget.y+rTarget.Hgt) return false;
	return true;
}

void C4Rect::Intersect(const C4Rect &r2)
{
	// Narrow bounds
	if (r2.x > x)
		if (r2.x + r2.Wdt < x + Wdt)
			{ x = r2.x; Wdt = r2.Wdt; }
		else
			{ Wdt -= (r2.x - x); x = r2.x; }
	else if (r2.x + r2.Wdt < x + Wdt)
		Wdt = r2.x + r2.Wdt - x;
	if (r2.y > y)
		if (r2.y + r2.Hgt < y + Hgt)
			{ y = r2.y; Hgt = r2.Hgt; }
		else
			{ Hgt -= (r2.y - y); y = r2.y; }
	else if (r2.y + r2.Hgt < y + Hgt)
		Hgt = r2.y + r2.Hgt - y;
	// Degenerated? Will happen when the two rects don't overlap
	if (Wdt < 0) Wdt = 0;
	if (Hgt < 0) Hgt = 0;
}

bool C4Rect::IntersectsLine(int32_t iX, int32_t iY, int32_t iX2, int32_t iY2)
{
	// Easy cases first
	if (Contains(iX, iY)) return true;
	if (Contains(iX2, iY2)) return true;
	if (iX < x && iX2 < x) return false;
	if (iY < y && iY2 < y) return false;
	if (iX >= x+Wdt && iX2 >= x+Wdt) return false;
	if (iY >= y+Hgt && iY2 >= y+Hgt) return false;
	// check some special cases
	if (iX == iX2 || iY == iY2) return true;
	// Check intersection left/right
	int32_t iXI, iYI;
	iXI = (iX < x ? x : x+Wdt);
	iYI = iY + (iY2 - iY) * (iXI - iX) / (iX2 - iX);
	if (iYI >= y && iYI < y+Hgt) return true;
	// Check intersection up/down
	iYI = (iY < y ? y : y+Hgt);
	iXI = iX + (iX2 - iX) * (iYI - iY) / (iY2 - iY);
	return iXI >= x && iXI < x+Wdt;
}

void C4Rect::Add(const C4Rect &r2)
{
	// Null? Don't do anything
	if (!r2.Wdt || !r2.Hgt) return;
	if (!Wdt || !Hgt)
	{
		*this = r2;
		return;
	}
	// Expand bounds
	if (r2.x < x)
		if (r2.x + r2.Wdt > x + Wdt)
			{ x = r2.x; Wdt = r2.Wdt; }
		else
			{ Wdt += (x - r2.x); x = r2.x; }
	else if (r2.x + r2.Wdt > x + Wdt)
		Wdt = r2.x + r2.Wdt - x;
	if (r2.y < y)
		if (r2.y + r2.Hgt > y + Hgt)
			{ y = r2.y; Hgt = r2.Hgt; }
		else
			{ Hgt += (y - r2.y); y = r2.y; }
	else if (r2.y + r2.Hgt > y + Hgt)
		Hgt = r2.y + r2.Hgt - y;
}

// ---- C4RectList ----


void C4RectList::ClipByRect(const C4Rect &rClip)
{
	// split up all rectangles
	for (int32_t i = 0; i < GetCount(); ++i)
	{
		C4Rect *pTarget = &Get(i);
		// any overlap?
		if (rClip.x+rClip.Wdt <= pTarget->x) continue;
		if (rClip.y+rClip.Hgt <= pTarget->y) continue;
		if (rClip.x >= pTarget->x+pTarget->Wdt) continue;
		if (rClip.y >= pTarget->y+pTarget->Hgt) continue;
		// okay; split up rectangle
		// first split will just reduce the target rectangle size
		// if more splits are done, additional rectangles need to be added
		int32_t iSplitCount = 0, iOver; C4Rect rcThis(*pTarget);
		// clipped by right side
		if ((iOver=rcThis.x+rcThis.Wdt-rClip.x-rClip.Wdt)>0)
		{
			pTarget->x += pTarget->Wdt - iOver; pTarget->Wdt = iOver; rcThis.Wdt -= iOver;
			++iSplitCount;
		}
		// clipped by obttom side
		if ((iOver=rcThis.y+rcThis.Hgt-rClip.y-rClip.Hgt)>0)
		{
			if (iSplitCount) { AddRect(rcThis); pTarget = &Get(GetCount()-1); }
			pTarget->y += pTarget->Hgt - iOver; pTarget->Hgt = iOver; rcThis.Hgt -= iOver;
			++iSplitCount;
		}
		// clipped by left side
		if ((iOver=rClip.x-rcThis.x)>0)
		{
			if (iSplitCount) { AddRect(rcThis); pTarget = &Get(GetCount()-1); }
			pTarget->Wdt = iOver; rcThis.Wdt -= iOver; rcThis.x = rClip.x;
			++iSplitCount;
		}
		// clipped by top side
		if ((iOver=rClip.y-rcThis.y)>0)
		{
			if (iSplitCount) { AddRect(rcThis); pTarget = &Get(GetCount()-1); }
			else ++iSplitCount;
			pTarget->Hgt = iOver; /* rcThis.Hgt -= iOver; rcThis.y = rClip.y; not needed, since rcThis is no longer used */
		}
		// nothing split? This means this rectnagle is completely contained
		if (!iSplitCount)
		{
			// make it vanish
			RemoveIndexedRect(i); --i;
		}
	}
	// concat rectangles if possible
	bool fDone = false;
	while (!fDone)
	{
		fDone=true;
		for (int32_t i = 0, cnt=GetCount(); i < cnt && fDone; ++i)
		{
			C4Rect &rc1 = Get(i);
			for (int32_t j = i+1; j < cnt; ++j)
			{
				C4Rect &rc2 = Get(j);
				if (rc1.y == rc2.y && rc1.Hgt == rc2.Hgt)
				{
					if (rc1.x + rc1.Wdt == rc2.x)
					{
						rc1.Wdt += rc2.Wdt; RemoveIndexedRect(j); fDone=false; break;
					}
					else if (rc2.x + rc2.Wdt == rc1.x)
					{
						rc2.Wdt += rc1.Wdt; RemoveIndexedRect(i); fDone=false; break;
					}
				}
				else if (rc1.x == rc2.x && rc1.Wdt == rc2.Wdt)
				{
					if (rc1.y + rc1.Hgt == rc2.y)
					{
						rc1.Hgt += rc2.Hgt; RemoveIndexedRect(j); fDone=false; break;
					}
					else if (rc2.y + rc2.Hgt == rc1.y)
					{
						rc2.Hgt += rc1.Hgt; RemoveIndexedRect(i); fDone=false; break;
					}
				}
			}
		}
	}
}

