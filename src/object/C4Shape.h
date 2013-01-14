/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2007  Matthes Bender
 * Copyright (c) 2001, 2003-2006  Sven Eberhardt
 * Copyright (c) 2005-2006  Peter Wortmann
 * Copyright (c) 2006, 2009  Günther Brammer
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

#ifndef INC_C4Shape
#define INC_C4Shape

#include "C4FacetEx.h"
#include "C4Constants.h"
#include "C4Rect.h"

#define C4D_VertexCpyPos (C4D_MaxVertex/2)

// a functional class to provide density for coordinates
class C4DensityProvider
{
public:
	virtual int32_t GetDensity(int32_t x, int32_t y) const;
	virtual ~C4DensityProvider() {}
};

extern C4DensityProvider DefaultDensityProvider;

class C4Shape: public C4Rect
{
public:
	C4Shape();
public:
	// remember to adjust C4Shape::CopyFrom and CreateOwnOriginalCopy when adding members here!
	int32_t FireTop;
	int32_t VtxNum;
	int32_t VtxX[C4D_MaxVertex];
	int32_t VtxY[C4D_MaxVertex];
	int32_t VtxCNAT[C4D_MaxVertex];
	int32_t VtxFriction[C4D_MaxVertex];
	int32_t ContactDensity;
	int32_t ContactCNAT;
	int32_t ContactCount;
	int32_t AttachMat;
	int32_t VtxContactCNAT[C4D_MaxVertex];
	int32_t VtxContactMat[C4D_MaxVertex];
	int32_t iAttachX, iAttachY, iAttachVtx;
public:
	void Default();
	void Clear();
	void Rotate(int32_t iAngle, bool bUpdateVertices);
	void Stretch(int32_t iCon, bool bUpdateVertices);
	void Jolt(int32_t iCon, bool bUpdateVertices);
	void GetVertexOutline(C4Rect &rRect);
	int32_t GetVertexY(int32_t iVertex);
	int32_t GetVertexX(int32_t iVertex);
	int32_t GetX() { return x; }
	int32_t GetY() { return y; }
	bool AddVertex(int32_t iX, int32_t iY);
	bool CheckContact(int32_t cx, int32_t cy);
	bool ContactCheck(int32_t cx, int32_t cy, uint32_t *border_hack_contacts=0);
	bool Attach(int32_t &cx, int32_t &cy, BYTE cnat_pos);
	bool LineConnect(int32_t tx, int32_t ty, int32_t cvtx, int32_t ld, int32_t oldx, int32_t oldy);
	bool InsertVertex(int32_t iPos, int32_t tx, int32_t ty);
	bool RemoveVertex(int32_t iPos);
	void CopyFrom(C4Shape rFrom, bool bCpyVertices, bool fCopyVerticesFromSelf);
	int32_t GetBottomVertex();
	int GetBottom(); // return lowest vertex Y
	int32_t GetVertexContact(int32_t iVtx, DWORD dwCheckMask, int32_t tx, int32_t ty, const C4DensityProvider &rDensityProvider = DefaultDensityProvider); // get CNAT-mask for given vertex - does not check range for iVtx!
	bool CheckScaleToWalk(int x, int y);
	void CreateOwnOriginalCopy(C4Shape &rFrom); // create copy of all vertex members in back area of own buffers
	void CompileFunc(StdCompiler *pComp, bool fRuntime);
};

#endif // INC_C4Shape
