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

#ifndef INC_C4Shape
#define INC_C4Shape

#include "config/C4Constants.h"
#include "lib/C4Rect.h"

#define C4D_VertexCpyPos (C4D_MaxVertex/2)

// a functional class to provide density for coordinates
class C4DensityProvider
{
public:
	virtual int32_t GetDensity(int32_t x, int32_t y) const;
	virtual ~C4DensityProvider() {}
};

extern C4DensityProvider DefaultDensityProvider;

class C4Shape : public C4Rect
{
public:
	// remember to adjust C4Shape::CopyFrom and CreateOwnOriginalCopy when adding members here!
	int32_t FireTop = 0;
	int32_t VtxNum = 0;
	int32_t VtxX[C4D_MaxVertex] = { 0 };
	int32_t VtxY[C4D_MaxVertex] = { 0 };
	int32_t VtxCNAT[C4D_MaxVertex] = { 0 };
	int32_t VtxFriction[C4D_MaxVertex] = { 0 };
	int32_t ContactDensity = C4M_Solid;
	int32_t ContactCNAT = 0;
	int32_t ContactCount = 0;
	int32_t AttachMat = MNone;
	int32_t VtxContactCNAT[C4D_MaxVertex] = { 0 };
	int32_t VtxContactMat[C4D_MaxVertex] = { 0 };
	int32_t iAttachX = 0, iAttachY = 0, iAttachVtx = 0;
public:
	void Default();
	void Rotate(C4Real Angle, bool bUpdateVertices);
	void Stretch(int32_t iCon, bool bUpdateVertices);
	void Jolt(int32_t iCon, bool bUpdateVertices);
	void GetVertexOutline(C4Rect &rRect);
	int32_t GetVertexY(int32_t iVertex);
	int32_t GetVertexX(int32_t iVertex);
	int32_t GetX() const { return x; }
	int32_t GetY() const { return y; }
	bool AddVertex(int32_t iX, int32_t iY);
	bool CheckContact(int32_t cx, int32_t cy);
	bool ContactCheck(int32_t cx, int32_t cy, uint32_t *border_hack_contacts=0, bool collide_halfvehic=false);
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
	void CompileFunc(StdCompiler *pComp, const C4Shape *default_shape);
private:
	bool CheckTouchableMaterial(int32_t x, int32_t y, int32_t vtx_i, int32_t y_dir = 0, const C4DensityProvider &rDensityProvider = DefaultDensityProvider);
};

#endif // INC_C4Shape
