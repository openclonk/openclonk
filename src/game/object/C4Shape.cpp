/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2005, 2007  Matthes Bender
 * Copyright (c) 2001-2007  Sven Eberhardt
 * Copyright (c) 2003, 2005-2007  Peter Wortmann
 * Copyright (c) 2006-2008  Günther Brammer
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

#include <C4Include.h>
#include <C4Shape.h>

#include <C4Physics.h>
#include <C4Material.h>
#include <C4Landscape.h>
#include <C4Record.h>

bool C4Shape::AddVertex(int32_t iX, int32_t iY)
	{
	if (VtxNum>=C4D_MaxVertex) return false;
	VtxX[VtxNum]=iX; VtxY[VtxNum]=iY;
	VtxNum++;
	return true;
	}

void C4Shape::Default()
	{
	ZeroMem(this,sizeof (C4Shape));
	AttachMat=MNone;
	ContactDensity=C4M_Solid;
	}

C4Shape::C4Shape()
	{
	Default();
	}

void C4Shape::Clear()
	{
	ZeroMem(this, sizeof (C4Shape));
	}

void C4Shape::Rotate(int32_t iAngle, bool bUpdateVertices)
  {
#ifdef DEBUGREC
	C4RCRotVtx rc;
	rc.x=x; rc.y=y; rc.wdt=Wdt; rc.hgt=Hgt; rc.r=iAngle;
	int32_t i = 0;
	for (; i<4; ++i)
		{ rc.VtxX[i]=VtxX[i]; rc.VtxY[i]=VtxY[i]; }
	AddDbgRec(RCT_RotVtx1, &rc, sizeof(rc));
#endif
  int32_t cnt,nvtx,nvty,rdia;

	//int32_t *vtx=VtxX;
	//int32_t *vty=VtxY;
  FIXED mtx[4];
	FIXED fAngle = itofix(iAngle);

	if (bUpdateVertices)
		{

		// Calculate rotation matrix
		mtx[0]=Cos(fAngle); mtx[1]=-Sin(fAngle);
		mtx[2]=-mtx[1]; mtx[3]= mtx[0];
		// Rotate vertices
		for (cnt=0; cnt<VtxNum; cnt++)
	    {
			//nvtx= (int32_t) ( mtx[0]*vtx[cnt] + mtx[1]*vty[cnt] );
			//nvty= (int32_t) ( mtx[2]*vtx[cnt] + mtx[3]*vty[cnt] );
			nvtx = fixtoi(mtx[0]*VtxX[cnt] + mtx[1]*VtxY[cnt]);
			nvty = fixtoi(mtx[2]*VtxX[cnt] + mtx[3]*VtxY[cnt]);
			VtxX[cnt]=nvtx; VtxY[cnt]=nvty;
			}

		/* This is freaking nuts. I used the int32_t* to shortcut the
			two int32_t arrays Shape.Vtx_[]. Without modifications to
			this code, after rotation the x-values of vertex 2 and 4
			are screwed to that of vertex 0. Direct use of the array
			variables instead of the pointers helped. Later in
			development, again without modification to this code, the
			same error occured again. I moved back to pointer array
			shortcut and it worked again. ?!

			The error occurs after the C4DefCore structure has
			changed. It must have something to do with struct
			member alignment. But why does pointer usage vs. array
			index make a difference?
		*/

		}

  // Enlarge Rect
  rdia= (int32_t) sqrt(double(x*x+y*y)) + 2;
  x=-rdia;
  y=-rdia;
  Wdt=2*rdia;
  Hgt=2*rdia;
#ifdef DEBUGREC
	rc.x=x; rc.y=y; rc.wdt=Wdt; rc.hgt=Hgt;
	for (i=0; i<4; ++i)
		{ rc.VtxX[i]=VtxX[i]; rc.VtxY[i]=VtxY[i]; }
	AddDbgRec(RCT_RotVtx2, &rc, sizeof(rc));
#endif
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


bool C4Shape::Attach(int32_t &cx, int32_t &cy, BYTE cnat_pos)
  {
	// Adjust given position to one pixel before contact
	// at vertices matching CNAT request.

  bool fAttached=false;


  int32_t vtx,xcnt,ycnt,xcrng,ycrng,xcd,ycd;
	BYTE cpix;

	// reset attached material
	AttachMat=MNone;

	// New attachment behaviour in CE:
	// Before, attachment was done by searching through all vertices,
	// and doing attachment to any vertex with a matching CNAT.
	// While this worked well for normal Clonk attachment, it caused nonsense
	// behaviour if multiple vertices matched the same CNAT. In effect, attachment
	// was then done to the last vertex only, usually stucking the object sooner
	// or later.
	// For instance, the scaling procedure of regular Clonks uses two CNAT_Left-
	// vertices (shoulder+belly), which "block" each other in situations like
	// scaling up battlements of towers. That way, the 2px-overhang of the
	// battlement is sufficient for keeping out scaling Clonks. The drawback is
	// that sometimes Clonks get stuck scaling in very sharp edges or single
	// floating material pixels; occuring quite often in Caverace, or maps where
	// you blast Granite and many single pixels remain.

	// determine attachment direction
	xcd=ycd=0;
	switch (cnat_pos & (~CNAT_Flags))
		{
		case CNAT_Top:    ycd=-1; break;
		case CNAT_Bottom: ycd=+1; break;
		case CNAT_Left:   xcd=-1; break;
		case CNAT_Right:  xcd=+1; break;
		}
	// check within attachment range
	xcrng=AttachRange*xcd*(-1); ycrng=AttachRange*ycd*(-1);
	for (xcnt=xcrng,ycnt=ycrng; (xcnt!=-xcrng) || (ycnt!=-ycrng); xcnt+=xcd,ycnt+=ycd)
		// check all vertices with matching CNAT
		for (vtx=0; vtx<VtxNum; vtx++)
			if (VtxCNAT[vtx] & cnat_pos)
				{
				// get new vertex pos
				int32_t ax=cx+VtxX[vtx]+xcnt+xcd, ay=cy+VtxY[vtx]+ycnt+ycd;
				// can attach here?
				cpix=GBackPix(ax,ay);
				if (MatDensity(PixCol2Mat(cpix)) >= ContactDensity)
					{
					// store attachment material
					AttachMat=PixCol2Mat(cpix);
					// store absolute attachment position
					iAttachX=ax; iAttachY=ay;
					iAttachVtx=vtx;
					// move position here
					cx+=xcnt; cy+=ycnt;
					// mark attachment
					fAttached=1;
					// break both looops
					xcnt=-xcrng-xcd; ycnt=-ycrng-ycd;
					break;
					}
				}
	  
  return fAttached;
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
			if (GBackDensity(cx+VtxX[cvtx],cy+VtxY[cvtx]) >= ContactDensity)
				return true;


  return false;
  }

bool C4Shape::ContactCheck(int32_t cx, int32_t cy)
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
			VtxContactMat[cvtx]=GBackMat(cx+VtxX[cvtx],cy+VtxY[cvtx]);

			if (GBackDensity(cx+VtxX[cvtx],cy+VtxY[cvtx]) >= ContactDensity)
	      {
	      ContactCNAT |= VtxCNAT[cvtx];
	      VtxContactCNAT[cvtx]|=CNAT_Center;
	      ContactCount++;
	      // Vertex center contact, now check top,bottom,left,right
	      if (GBackDensity(cx+VtxX[cvtx],cy+VtxY[cvtx]-1) >= ContactDensity)
	        VtxContactCNAT[cvtx]|=CNAT_Top;
	      if (GBackDensity(cx+VtxX[cvtx],cy+VtxY[cvtx]+1) >= ContactDensity)
	        VtxContactCNAT[cvtx]|=CNAT_Bottom;
	      if (GBackDensity(cx+VtxX[cvtx]-1,cy+VtxY[cvtx]) >= ContactDensity)
	        VtxContactCNAT[cvtx]|=CNAT_Left;
	      if (GBackDensity(cx+VtxX[cvtx]+1,cy+VtxY[cvtx]) >= ContactDensity)
	        VtxContactCNAT[cvtx]|=CNAT_Right;
	      }
	    }


  return !!ContactCount;
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
		VtxNum = (fCopyVerticesFromSelf ? Min<int32_t>(VtxNum, C4D_VertexCpyPos) : rFrom.VtxNum);
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
	for(int32_t i = 0; i < VtxNum; i++)
		if(VtxCNAT[i] & CNAT_Bottom)
			if(iMax == -1 || VtxY[i] < VtxY[iMax])
				iMax = i;
	return iMax;
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
		if (dwCheckMask & CNAT_Center) if (rDensityProvider.GetDensity(tx, ty) >= ContactDensity)   iContact |= CNAT_Center;
		if (dwCheckMask & CNAT_Left)   if (rDensityProvider.GetDensity(tx-1, ty) >= ContactDensity) iContact |= CNAT_Left;
		if (dwCheckMask & CNAT_Right)  if (rDensityProvider.GetDensity(tx+1, ty) >= ContactDensity) iContact |= CNAT_Right;
		if (dwCheckMask & CNAT_Top)    if (rDensityProvider.GetDensity(tx, ty-1) >= ContactDensity) iContact |= CNAT_Top;
		if (dwCheckMask & CNAT_Bottom) if (rDensityProvider.GetDensity(tx, ty+1) >= ContactDensity) iContact |= CNAT_Bottom;
		}
	// return resulting bitmask
	return iContact;
	}

void C4Shape::CreateOwnOriginalCopy(C4Shape &rFrom)
	{
	// copy vertices from original buffer, including count
	VtxNum = Min<int32_t>(rFrom.VtxNum, C4D_VertexCpyPos);
	memcpy(VtxX+C4D_VertexCpyPos, rFrom.VtxX, VtxNum*sizeof(*VtxX));
	memcpy(VtxY+C4D_VertexCpyPos, rFrom.VtxY, VtxNum*sizeof(*VtxY));
	memcpy(VtxCNAT+C4D_VertexCpyPos, rFrom.VtxCNAT, VtxNum*sizeof(*VtxCNAT));
	memcpy(VtxFriction+C4D_VertexCpyPos, rFrom.VtxFriction, VtxNum*sizeof(*VtxFriction));
	memcpy(VtxContactCNAT+C4D_VertexCpyPos, rFrom.VtxContactCNAT, VtxNum*sizeof(*VtxContactCNAT));
	memcpy(VtxContactMat+C4D_VertexCpyPos, rFrom.VtxContactMat, VtxNum*sizeof(*VtxContactMat));
	}

void C4Shape::CompileFunc(StdCompiler *pComp, bool fRuntime)
	{
	// Note: Compiled directly into "Object" and "DefCore"-categories, so beware of name clashes
	// (see C4Object::CompileFunc and C4Def::CompileFunc)
	pComp->Value(mkNamingAdapt( Wdt,												"Width",							0									));
	pComp->Value(mkNamingAdapt( Hgt,												"Height",							0									));
	pComp->Value(mkNamingAdapt( mkArrayAdapt(&x,2,0),				"Offset"																));
	pComp->Value(mkNamingAdapt( VtxNum,											"Vertices",						0									));
	pComp->Value(mkNamingAdapt( toC4CArr(VtxX),							"VertexX"																));
	pComp->Value(mkNamingAdapt( toC4CArr(VtxY),							"VertexY"																));
	pComp->Value(mkNamingAdapt( toC4CArr(VtxCNAT),			    "VertexCNAT"														));
	pComp->Value(mkNamingAdapt( toC4CArr(VtxFriction),			"VertexFriction"												));
	pComp->Value(mkNamingAdapt( ContactDensity,							"ContactDensity",			C4M_Solid					));
	pComp->Value(mkNamingAdapt( FireTop,										"FireTop",						0									));
	if(fRuntime)
		{
		pComp->Value(mkNamingAdapt( iAttachX,									  "AttachX",						0									));
		pComp->Value(mkNamingAdapt( iAttachY,										"AttachY",						0									));
		pComp->Value(mkNamingAdapt( iAttachVtx,									"AttachVtx",					0									));
		}
	}
