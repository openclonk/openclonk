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

/* Basic classes for rectangles and vertex outlines */

#include "C4Include.h"
#include "object/C4Shape.h"

#include "game/C4Physics.h"
#include "landscape/C4Material.h"
#include "landscape/C4Landscape.h"
#include "control/C4Record.h"

bool C4Shape::AddVertex(int32_t iX, int32_t iY)
{
	if (VtxNum>=C4D_MaxVertex) return false;
	VtxX[VtxNum]=iX; VtxY[VtxNum]=iY;
	VtxNum++;
	return true;
}

void C4Shape::Default()
{
	InplaceReconstruct(this);
}

void C4Shape::Rotate(C4Real Angle, bool bUpdateVertices)
{
	C4RCRotVtx rc;
	int32_t i = 0;
	if (Config.General.DebugRec)
	{
		rc.x=x; rc.y=y; rc.wdt=Wdt; rc.hgt=Hgt; rc.r=fixtoi(Angle);
		for (; i<4; ++i)
			{ rc.VtxX[i]=VtxX[i]; rc.VtxY[i]=VtxY[i]; }
		AddDbgRec(RCT_RotVtx1, &rc, sizeof(rc));
	}
	int32_t cnt,nvtx,nvty,nwdt,nhgt;

	C4Real mtx[4];

	// Calculate rotation matrix
	mtx[0] = Cos(Angle); mtx[1] = -Sin(Angle);
	mtx[2] = -mtx[1];     mtx[3] = mtx[0];

	if (bUpdateVertices)
	{
		// Rotate vertices
		for (cnt = 0; cnt < VtxNum; cnt++)
		{
			nvtx = fixtoi(mtx[0] * VtxX[cnt] + mtx[1] * VtxY[cnt]);
			nvty = fixtoi(mtx[2] * VtxX[cnt] + mtx[3] * VtxY[cnt]);
			VtxX[cnt] = nvtx; VtxY[cnt] = nvty;
		}
	}

	// Enlarge Rect
	nvtx = fixtoi(mtx[0] * x + mtx[1] * y);
	nvty = fixtoi(mtx[2] * x + mtx[3] * y);
	if (mtx[0] > 0)
	{
		if (mtx[1] > 0)
		{
			nwdt = fixtoi(mtx[0] * Wdt + mtx[1] * Hgt);
			nhgt = fixtoi(mtx[1] * Wdt + mtx[0] * Hgt);
			x = nvtx;
			y = nvty - fixtoi(mtx[1] * Wdt);
		}
		else
		{
			nwdt = fixtoi(mtx[0] * Wdt - mtx[1] * Hgt);
			nhgt = fixtoi(- mtx[1] * Wdt + mtx[0] * Hgt);
			x = nvtx + fixtoi(mtx[1] * Hgt);
			y = nvty;
		}
	}
	else
	{
		if (mtx[1] > 0)
		{
			nwdt = fixtoi(- mtx[0] * Wdt + mtx[1] * Hgt);
			nhgt = fixtoi(mtx[1] * Wdt - mtx[0] * Hgt);
			x = nvtx + fixtoi(mtx[0] * Wdt);
			y = nvty - nhgt;
		}
		else
		{
			nwdt = fixtoi(- mtx[0] * Wdt - mtx[1] * Hgt);
			nhgt = fixtoi(- mtx[1] * Wdt - mtx[0] * Hgt);
			x = nvtx - nwdt;
			y = nvty + fixtoi(mtx[0] * Hgt);
		}
	}
	Wdt = nwdt;
	Hgt = nhgt;
	if (Config.General.DebugRec)
	{
		rc.x=x; rc.y=y; rc.wdt=Wdt; rc.hgt=Hgt;
		for (i=0; i<4; ++i)
			{ rc.VtxX[i]=VtxX[i]; rc.VtxY[i]=VtxY[i]; }
		AddDbgRec(RCT_RotVtx2, &rc, sizeof(rc));
	}
}

void C4Shape::Stretch(int32_t iCon, bool bUpdateVertices)
{
	int32_t cnt;
	x=x*iCon/FullCon;
	y=y*iCon/FullCon;
	Wdt=Wdt*iCon/FullCon;
	Hgt=Hgt*iCon/FullCon;
	FireTop=FireTop*iCon/FullCon;
	if (bUpdateVertices)
		for (cnt=0; cnt<VtxNum; cnt++)
		{
			VtxX[cnt]=VtxX[cnt]*iCon/FullCon;
			VtxY[cnt]=VtxY[cnt]*iCon/FullCon;
		}
}

void C4Shape::Jolt(int32_t iCon, bool bUpdateVertices)
{
	int32_t cnt;
	y=y*iCon/FullCon;
	Hgt=Hgt*iCon/FullCon;
	FireTop=FireTop*iCon/FullCon;
	if (bUpdateVertices)
		for (cnt=0; cnt<VtxNum; cnt++)
			VtxY[cnt]=VtxY[cnt]*iCon/FullCon;
}

void C4Shape::GetVertexOutline(C4Rect &rRect)
{
	int32_t cnt;
	rRect.x=rRect.y=rRect.Wdt=rRect.Hgt=0;
	for (cnt=0; cnt<VtxNum; cnt++)
	{
		// Extend left
		if (VtxX[cnt]<rRect.x)
		{
			rRect.Wdt+=rRect.x-VtxX[cnt];
			rRect.x=VtxX[cnt];
		}
		// Extend right
		else if (VtxX[cnt]>rRect.x+rRect.Wdt)
			{ rRect.Wdt=VtxX[cnt]-rRect.x; }

		// Extend up
		if (VtxY[cnt]<rRect.y)
		{
			rRect.Hgt+=rRect.y-VtxY[cnt];
			rRect.y=VtxY[cnt];
		}
		// Extend down
		else if (VtxY[cnt]>rRect.y+rRect.Hgt)
			{ rRect.Hgt=VtxY[cnt]-rRect.y; }
	}

	rRect.Hgt+=rRect.y-y;
	rRect.y=y;

}

inline bool C4Shape::CheckTouchableMaterial(int32_t x, int32_t y, int32_t vtx_i, int32_t ydir, const C4DensityProvider &rDensityProvider) {
	return rDensityProvider.GetDensity(x,y) >= ContactDensity &&
	       ((ydir > 0 && !(CNAT_PhaseHalfVehicle & VtxCNAT[vtx_i])) || !IsMCHalfVehicle(::Landscape.GetPix(x,y)));
}

// Adjust given position to one pixel before contact
// at vertices matching CNAT request.
bool C4Shape::Attach(int32_t &cx, int32_t &cy, BYTE cnat_pos)
{
	// reset attached material
	AttachMat=MNone;
	int xcd = 0;
	int ycd = 0;
	// determine attachment direction
	switch (cnat_pos & (~CNAT_Flags))
	{
	case CNAT_Top:    ycd=-1; break;
	case CNAT_Bottom: ycd=+1; break;
	case CNAT_Left:   xcd=-1; break;
	case CNAT_Right:  xcd=+1; break;
	default: return false;
	}
	int testx = cx;
	int testy = cy;
	bool increase_distance = true;
	bool any_contact = false;
	// Find the nearest position that has at least one vertex adjacent to dense material
	// and no vertices in dense materials
	while (Abs(testx - cx) < AttachRange && Abs(testy - cy) < AttachRange)
	{
		bool found = false;
		for (int i = 0; i < VtxNum; ++i)
		{
			if (VtxCNAT[i] & cnat_pos)
			{
				// get new vertex pos
				int32_t ax = testx + VtxX[i], ay = testy + VtxY[i];
				if (CheckTouchableMaterial(ax, ay, i))
				{
					found = false;
					break;
				}
				// can attach here?
				if (CheckTouchableMaterial(ax + xcd, ay + ycd, i, ycd))
				{
					found = true;
					any_contact = true;
					// store attachment material
					AttachMat = GBackMat(ax + xcd, ay + ycd);
					// store absolute attachment position
					iAttachX = ax + xcd; iAttachY = ay + ycd;
					iAttachVtx = i;
				}

			}
		}
		if (found)
		{
			cx = testx;
			cy = testy;
			return true;
		}
		// Try positions in order of distance from the origin,
		// and alternating the direction
		testx = cx - (testx - cx);
		testy = cy - (testy - cy);
		if (increase_distance)
		{
			testx += xcd;
			testy += ycd;
		}
		increase_distance = !increase_distance;
	}
	return any_contact;
}


bool C4Shape::LineConnect(int32_t tx, int32_t ty, int32_t cvtx, int32_t ld, int32_t oldx, int32_t oldy)
{

	if (VtxNum<2) return false;

	// No modification
	if ((VtxX[cvtx]==tx) && (VtxY[cvtx]==ty)) return true;

	// Check new path
	int32_t ix,iy;
	if (PathFree(tx,ty,VtxX[cvtx+ld],VtxY[cvtx+ld],&ix,&iy))
	{
		// Okay, set vertex
		VtxX[cvtx]=tx; VtxY[cvtx]=ty;
		return true;
	}
	else
	{
		// Intersected, find bend vertex
		bool found = false;
		int32_t cix;
		int32_t ciy;
		for (int irange = 4; irange <= 12; irange += 4)
			for (cix = ix - irange / 2; cix <= ix + irange; cix += irange)
				for (ciy = iy - irange / 2; ciy <= iy + irange; ciy += irange)
				{
					if (PathFree(cix,ciy,tx,ty) && PathFree(cix,ciy,VtxX[cvtx+ld],VtxY[cvtx+ld]))
					{
						found = true;
						goto out;
					}
				}
out:
		if (!found)
		{
			// try bending directly at path the line took
			// allow going through vehicle in this case to allow lines through castles and elevator shafts
			cix = oldx;
			ciy = oldy;
			if (!PathFreeIgnoreVehicle(cix,ciy,tx,ty) || !PathFreeIgnoreVehicle(cix,ciy,VtxX[cvtx+ld],VtxY[cvtx+ld]))
				if (!PathFreeIgnoreVehicle(cix,ciy,tx,ty) || !PathFreeIgnoreVehicle(cix,ciy,VtxX[cvtx+ld],VtxY[cvtx+ld]))
					return false; // Found no bend vertex
		}
		// Insert bend vertex
		if (ld>0)
		{
			if (!InsertVertex(cvtx+1,cix,ciy)) return false;
		}
		else
		{
			if (!InsertVertex(cvtx,cix,ciy)) return false;
			cvtx++;
		}
		// Okay, set vertex
		VtxX[cvtx]=tx; VtxY[cvtx]=ty;
		return true;
	}

	return false;
}

bool C4Shape::InsertVertex(int32_t iPos, int32_t tx, int32_t ty)
{
	if (VtxNum+1>C4D_MaxVertex) return false;
	if (iPos < 0 || iPos > VtxNum) return false;
	// Insert vertex before iPos
	for (int32_t cnt=VtxNum; cnt>iPos; cnt--)
		{ VtxX[cnt]=VtxX[cnt-1]; VtxY[cnt]=VtxY[cnt-1]; }
	VtxX[iPos]=tx; VtxY[iPos]=ty;
	VtxNum++;
	return true;
}

bool C4Shape::RemoveVertex(int32_t iPos)
{
	if (!Inside<int32_t>(iPos,0,VtxNum-1)) return false;
	for (int32_t cnt=iPos; cnt+1<VtxNum; cnt++)
		{ VtxX[cnt]=VtxX[cnt+1]; VtxY[cnt]=VtxY[cnt+1]; }
	VtxNum--;
	return true;
}

bool C4Shape::CheckContact(int32_t cx, int32_t cy)
{
	// Check all vertices at given object position.
	// Return true on any contact.


	for (int32_t cvtx=0; cvtx<VtxNum; cvtx++)
		if (!(VtxCNAT[cvtx] & CNAT_NoCollision))
			if (CheckTouchableMaterial(cx+VtxX[cvtx],cy+VtxY[cvtx], cvtx))
				return true;


	return false;
}

bool C4Shape::ContactCheck(int32_t cx, int32_t cy, uint32_t *border_hack_contacts, bool collide_halfvehic)
{
	// Check all vertices at given object position.
	// Set ContactCNAT and ContactCount.
	// Set VtxContactCNAT and VtxContactMat.
	// Return true on any contact.


	ContactCNAT=CNAT_None;
	ContactCount=0;

	for (int32_t cvtx=0; cvtx<VtxNum; cvtx++)

		// Ignore vertex if collision has been flagged out
		if (!(VtxCNAT[cvtx] & CNAT_NoCollision))

		{
			VtxContactCNAT[cvtx]=CNAT_None;
			int32_t x = cx+VtxX[cvtx];
			int32_t y = cy+VtxY[cvtx];
			VtxContactMat[cvtx]=GBackMat(x,y);

			if (CheckTouchableMaterial(x, y, cvtx, collide_halfvehic? 1:0))
			{
				ContactCNAT |= VtxCNAT[cvtx];
				VtxContactCNAT[cvtx]|=CNAT_Center;
				ContactCount++;
				// Vertex center contact, now check top,bottom,left,right
				if (CheckTouchableMaterial(x,y-1,cvtx, collide_halfvehic ? 1 : 0))
					VtxContactCNAT[cvtx]|=CNAT_Top;
				if (CheckTouchableMaterial(x,y+1,cvtx, collide_halfvehic ? 1 : 0))
					VtxContactCNAT[cvtx]|=CNAT_Bottom;
				if (CheckTouchableMaterial(x-1,y,cvtx, collide_halfvehic ? 1 : 0))
					VtxContactCNAT[cvtx]|=CNAT_Left;
				if (CheckTouchableMaterial(x+1,y,cvtx, collide_halfvehic ? 1 : 0))
					VtxContactCNAT[cvtx]|=CNAT_Right;
			}
			if (border_hack_contacts)
			{
				if (x == 0 && CheckTouchableMaterial(x-1, y, cvtx)) *border_hack_contacts |= CNAT_Left;
				else if (x == ::Landscape.GetWidth() && CheckTouchableMaterial(x+1, y, cvtx)) *border_hack_contacts |= CNAT_Right;
			}
		}


	return !!ContactCount;
}

bool C4Shape::CheckScaleToWalk(int x, int y)
{
	for (int32_t i = 0; i < VtxNum; i++)
	{
		if (VtxCNAT[i] & CNAT_NoCollision)
			continue;
		if (VtxCNAT[i] & CNAT_Bottom)
		{
			// no ground under the feet?
			if (CheckTouchableMaterial(x + VtxX[i], y + VtxY[i] + 1, i))
				return false;
		}
		else
		{
			// can climb with hands?
			if (CheckTouchableMaterial(x + VtxX[i] - 1, y + VtxY[i], i))
				return false;
			if (CheckTouchableMaterial(x + VtxX[i] + 1, y + VtxY[i], i))
				return false;
		}
	}
	return true;
}

int32_t C4Shape::GetVertexX(int32_t iVertex)
{
	if (!Inside<int32_t>(iVertex,0,VtxNum-1)) return 0;
	return VtxX[iVertex];
}

int32_t C4Shape::GetVertexY(int32_t iVertex)
{
	if (!Inside<int32_t>(iVertex,0,VtxNum-1)) return 0;
	return VtxY[iVertex];
}

void C4Shape::CopyFrom(C4Shape rFrom, bool bCpyVertices, bool fCopyVerticesFromSelf)
{
	if (bCpyVertices)
	{
		// truncate / copy vertex count
		VtxNum = (fCopyVerticesFromSelf ? std::min<int32_t>(VtxNum, C4D_VertexCpyPos) : rFrom.VtxNum);
		// restore vertices from back of own buffer (retaining count)
		int32_t iCopyPos = (fCopyVerticesFromSelf ? C4D_VertexCpyPos : 0);
		C4Shape &rVtxFrom = (fCopyVerticesFromSelf ? *this : rFrom);
		memcpy(VtxX, rVtxFrom.VtxX+iCopyPos, VtxNum*sizeof(*VtxX));
		memcpy(VtxY, rVtxFrom.VtxY+iCopyPos, VtxNum*sizeof(*VtxY));
		memcpy(VtxCNAT, rVtxFrom.VtxCNAT+iCopyPos, VtxNum*sizeof(*VtxCNAT));
		memcpy(VtxFriction, rVtxFrom.VtxFriction+iCopyPos, VtxNum*sizeof(*VtxFriction));
		memcpy(VtxContactCNAT, rVtxFrom.VtxContactCNAT+iCopyPos, VtxNum*sizeof(*VtxContactCNAT));
		memcpy(VtxContactMat, rVtxFrom.VtxContactMat+iCopyPos, VtxNum*sizeof(*VtxContactMat));
		// continue: copies other members
	}
	*((C4Rect *) this) = rFrom;
	AttachMat=rFrom.AttachMat;
	ContactCNAT=rFrom.ContactCNAT;
	ContactCount=rFrom.ContactCount;
	FireTop=rFrom.FireTop;
}

int32_t C4Shape::GetBottomVertex()
{
	// return bottom-most vertex
	int32_t iMax = -1;
	for (int32_t i = 0; i < VtxNum; i++)
		if (VtxCNAT[i] & CNAT_Bottom)
			if (iMax == -1 || VtxY[i] < VtxY[iMax])
				iMax = i;
	return iMax;
}

int C4Shape::GetBottom()
{
	int b = INT_MIN;
	for (int32_t i = 0; i < VtxNum; i++)
		if (~VtxCNAT[i] & CNAT_NoCollision)
			if (VtxY[i] > b)
				b = VtxY[i];
	if (b == INT_MIN)
		return y + Hgt;
	return b;
}

C4DensityProvider DefaultDensityProvider;

int32_t C4DensityProvider::GetDensity(int32_t x, int32_t y) const
{
	// default density provider checks the landscape
	return GBackDensity(x,y);
}

int32_t C4Shape::GetVertexContact(int32_t iVtx, DWORD dwCheckMask, int32_t tx, int32_t ty, const C4DensityProvider &rDensityProvider)
{
	// default check mask
	if (!dwCheckMask) dwCheckMask = VtxCNAT[iVtx];
	// check vertex positions (vtx num not range-checked!)
	tx += VtxX[iVtx]; ty += VtxY[iVtx];
	int32_t iContact = 0;
	// check all directions for solid mat
	if (~VtxCNAT[iVtx] & CNAT_NoCollision)
	{
		if (dwCheckMask & CNAT_Center) if (CheckTouchableMaterial(tx, ty  , iVtx, 0, rDensityProvider)) iContact |= CNAT_Center;
		if (dwCheckMask & CNAT_Left)   if (CheckTouchableMaterial(tx-1, ty, iVtx, 0, rDensityProvider)) iContact |= CNAT_Left;
		if (dwCheckMask & CNAT_Right)  if (CheckTouchableMaterial(tx+1, ty, iVtx, 0, rDensityProvider)) iContact |= CNAT_Right;
		if (dwCheckMask & CNAT_Top)    if (CheckTouchableMaterial(tx, ty-1, iVtx, 0, rDensityProvider)) iContact |= CNAT_Top;
		if (dwCheckMask & CNAT_Bottom) if (CheckTouchableMaterial(tx, ty+1, iVtx, 1, rDensityProvider)) iContact |= CNAT_Bottom;
	}
	// return resulting bitmask
	return iContact;
}

void C4Shape::CreateOwnOriginalCopy(C4Shape &rFrom)
{
	// copy vertices from original buffer, including count
	VtxNum = std::min<int32_t>(rFrom.VtxNum, C4D_VertexCpyPos);
	memcpy(VtxX+C4D_VertexCpyPos, rFrom.VtxX, VtxNum*sizeof(*VtxX));
	memcpy(VtxY+C4D_VertexCpyPos, rFrom.VtxY, VtxNum*sizeof(*VtxY));
	memcpy(VtxCNAT+C4D_VertexCpyPos, rFrom.VtxCNAT, VtxNum*sizeof(*VtxCNAT));
	memcpy(VtxFriction+C4D_VertexCpyPos, rFrom.VtxFriction, VtxNum*sizeof(*VtxFriction));
	memcpy(VtxContactCNAT+C4D_VertexCpyPos, rFrom.VtxContactCNAT, VtxNum*sizeof(*VtxContactCNAT));
	memcpy(VtxContactMat+C4D_VertexCpyPos, rFrom.VtxContactMat, VtxNum*sizeof(*VtxContactMat));
}

void C4Shape::CompileFunc(StdCompiler *pComp, const C4Shape *default_shape)
{
	const StdBitfieldEntry<int32_t> ContactDirections[] =
	{

		{ "CNAT_None", CNAT_None },
		{ "CNAT_Left", CNAT_Left },
		{ "CNAT_Right", CNAT_Right },
		{ "CNAT_Top", CNAT_Top },
		{ "CNAT_Bottom", CNAT_Bottom },
		{ "CNAT_Center", CNAT_Center },
		{ "CNAT_MultiAttach", CNAT_MultiAttach },
		{ "CNAT_NoCollision", CNAT_NoCollision },
		{ "CNAT_PhaseHalfVehicle", CNAT_PhaseHalfVehicle },

		{ nullptr, 0 }
	};

	// a default shape is given in object compilation context only
	bool fRuntime = !!default_shape;
	C4Shape default_def_shape;
	if (!default_shape) default_shape = &default_def_shape;
	// Note: Compiled directly into "Object" and "DefCore"-categories, so beware of name clashes
	// (see C4Object::CompileFunc and C4Def::CompileFunc)
	pComp->Value(mkNamingAdapt( Wdt,                        "Width",              default_shape->Wdt));
	pComp->Value(mkNamingAdapt( Hgt,                        "Height",             default_shape->Hgt));
	pComp->Value(mkNamingAdapt( mkArrayAdaptDefArr(&x,2,&default_shape->x),               "Offset",             &default_shape->x));
	pComp->Value(mkNamingAdapt( VtxNum,                                                   "Vertices",           default_shape->VtxNum));
	pComp->Value(mkNamingAdapt( mkArrayAdaptDMA(VtxX, default_shape->VtxX),               "VertexX",            default_shape->VtxX));
	pComp->Value(mkNamingAdapt( mkArrayAdaptDMA(VtxY, default_shape->VtxY),               "VertexY",            default_shape->VtxY));
	pComp->Value(mkNamingAdapt( mkArrayAdaptDMAM(VtxCNAT, default_shape->VtxCNAT, [&](decltype(*VtxCNAT) &elem){ return mkBitfieldAdapt<int32_t>(elem, ContactDirections); }), "VertexCNAT", default_shape->VtxCNAT));
	pComp->Value(mkNamingAdapt( mkArrayAdaptDMA(VtxFriction, default_shape->VtxFriction), "VertexFriction",     default_shape->VtxFriction));
	pComp->Value(mkNamingAdapt( ContactDensity,             "ContactDensity",     default_shape->ContactDensity));
	pComp->Value(mkNamingAdapt( FireTop,                    "FireTop",            default_shape->FireTop));
	if (fRuntime)
	{
		pComp->Value(mkNamingAdapt( iAttachX,                   "AttachX",            0                 ));
		pComp->Value(mkNamingAdapt( iAttachY,                   "AttachY",            0                 ));
		pComp->Value(mkNamingAdapt( iAttachVtx,                 "AttachVtx",          0                 ));
	}
}
