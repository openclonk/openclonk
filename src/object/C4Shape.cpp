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

#include "control/C4Record.h"
#include "game/C4Physics.h"
#include "landscape/C4Landscape.h"
#include "landscape/C4Material.h"

bool C4Shape::AddVertex(int32_t iX, int32_t iY)
{
	if (VtxNum >= C4D_MaxVertex)
	{
		return false;
	}
	VtxX[VtxNum] = iX;
	VtxY[VtxNum] = iY;
	VtxNum++;
	return true;
}

void C4Shape::Default()
{
	InplaceReconstruct(this);
}

void C4Shape::Rotate(C4Real angle, bool update_vertices)
{
	if (Config.General.DebugRec)
	{
		C4RCRotVtx rc;
		rc.x = x;
		rc.y = y;
		rc.wdt = Wdt;
		rc.hgt = Hgt;
		rc.r = fixtoi(angle);
		for (int32_t i = 0; i < 4; ++i)
		{
			rc.VtxX[i] = VtxX[i];
			rc.VtxY[i] = VtxY[i];
		}
		AddDbgRec(RCT_RotVtx1, &rc, sizeof(rc));
	}

	// Calculate rotation matrix
	C4Real rot_matrix[4];
	rot_matrix[0] = Cos(angle);  rot_matrix[1] = -Sin(angle);
	rot_matrix[2] = -rot_matrix[1];     rot_matrix[3] = rot_matrix[0];

	if (update_vertices)
	{
		// Rotate vertices
		for (int32_t cnt = 0; cnt < VtxNum; cnt++)
		{
			int32_t old_x = VtxX[cnt];
			int32_t old_y = VtxY[cnt];
			VtxX[cnt] = fixtoi(rot_matrix[0] * old_x + rot_matrix[1] * old_y);
			VtxY[cnt] = fixtoi(rot_matrix[2] * old_x + rot_matrix[3] * old_y);
		}
	}

	// Enlarge Rect
	int32_t new_x = fixtoi(rot_matrix[0] * x + rot_matrix[1] * y);
	int32_t new_y = fixtoi(rot_matrix[2] * x + rot_matrix[3] * y);
	int32_t new_wdt = 0;
	int32_t new_hgt = 0;
	if (rot_matrix[0] > 0)
	{
		if (rot_matrix[1] > 0)
		{
			new_wdt = fixtoi(rot_matrix[0] * Wdt + rot_matrix[1] * Hgt);
			new_hgt = fixtoi(rot_matrix[1] * Wdt + rot_matrix[0] * Hgt);
			x = new_x;
			y = new_y - fixtoi(rot_matrix[1] * Wdt);
		}
		else
		{
			new_wdt = fixtoi(rot_matrix[0] * Wdt - rot_matrix[1] * Hgt);
			new_hgt = fixtoi(- rot_matrix[1] * Wdt + rot_matrix[0] * Hgt);
			x = new_x + fixtoi(rot_matrix[1] * Hgt);
			y = new_y;
		}
	}
	else
	{
		if (rot_matrix[1] > 0)
		{
			new_wdt = fixtoi(- rot_matrix[0] * Wdt + rot_matrix[1] * Hgt);
			new_hgt = fixtoi(rot_matrix[1] * Wdt - rot_matrix[0] * Hgt);
			x = new_x + fixtoi(rot_matrix[0] * Wdt);
			y = new_y - new_hgt;
		}
		else
		{
			new_wdt = fixtoi(- rot_matrix[0] * Wdt - rot_matrix[1] * Hgt);
			new_hgt = fixtoi(- rot_matrix[1] * Wdt - rot_matrix[0] * Hgt);
			x = new_x - new_wdt;
			y = new_y + fixtoi(rot_matrix[0] * Hgt);
		}
	}
	Wdt = new_wdt;
	Hgt = new_hgt;
	if (Config.General.DebugRec)
	{
		C4RCRotVtx rc;
		rc.x = x;
		rc.y = y;
		rc.wdt = Wdt;
		rc.hgt = Hgt;
		for (int32_t i = 0; i < 4; ++i)
		{
			rc.VtxX[i] = VtxX[i];
			rc.VtxY[i] = VtxY[i];
		}
		AddDbgRec(RCT_RotVtx2, &rc, sizeof(rc));
	}
}

static inline int32_t ScaledByCon(int32_t value, int32_t con)
{
	return value * con / FullCon;
}

void C4Shape::Stretch(int32_t iCon, bool update_vertices)
{
	x = ScaledByCon(x, iCon);
	y = ScaledByCon(y, iCon);
	Wdt = ScaledByCon(Wdt, iCon);
	Hgt = ScaledByCon(Hgt, iCon);
	if (update_vertices)
	{
		for (int32_t i = 0; i < VtxNum; i++)
		{
			VtxX[i] = ScaledByCon(VtxX[i], iCon);
			VtxY[i] = ScaledByCon(VtxY[i], iCon);
		}
	}
}

void C4Shape::Jolt(int32_t iCon, bool update_vertices)
{
	y = ScaledByCon(y, iCon);
	Hgt = ScaledByCon(Hgt, iCon);
	if (update_vertices)
	{
		for (int32_t i = 0; i < VtxNum; i++)
		{
			VtxY[i] = ScaledByCon(VtxY[i], iCon);
		}
	}
}

void C4Shape::GetVertexOutline(C4Rect &rRect)
{
	rRect.x = 0;
	rRect.y = 0;
	rRect.Wdt = 0;
	rRect.Hgt = 0;
	for (int32_t i = 0; i < VtxNum; i++)
	{
		// Extend left
		if (VtxX[i] < rRect.x)
		{
			rRect.Wdt += rRect.x - VtxX[i];
			rRect.x = VtxX[i];
		}
		// Extend right
		else if (VtxX[i] > rRect.x + rRect.Wdt)
		{
			rRect.Wdt = VtxX[i] - rRect.x;
		}
		// Extend up
		if (VtxY[i] < rRect.y)
		{
			rRect.Hgt += rRect.y - VtxY[i];
			rRect.y = VtxY[i];
		}
		// Extend down
		else if (VtxY[i] > rRect.y + rRect.Hgt)
		{
			rRect.Hgt = VtxY[i] - rRect.y;
		}
	}

	rRect.Hgt += rRect.y - y;
	rRect.y = y;
}

inline bool C4Shape::CheckTouchableMaterial(int32_t x, int32_t y, int32_t vtx_i, int32_t ydir, const C4DensityProvider &rDensityProvider)
{
	return rDensityProvider.GetDensity(x, y) >= ContactDensity
		&& ((ydir > 0 && !(CNAT_PhaseHalfVehicle & VtxCNAT[vtx_i])) || !IsMCHalfVehicle(::Landscape.GetPix(x, y)));
}

// Adjust given position to one pixel before contact
// at vertices matching CNAT request.
bool C4Shape::Attach(int32_t &cx, int32_t &cy, BYTE cnat_pos)
{
	// Reset attached material
	AttachMat = MNone;
	int xcd = 0;
	int ycd = 0;
	// Determine attachment direction
	switch (cnat_pos & (~CNAT_Flags))
	{
	case CNAT_Top:
		ycd = -1;
		break;
	case CNAT_Bottom:
		ycd = +1;
		break;
	case CNAT_Left:
		xcd = -1;
		break;
	case CNAT_Right:
		xcd = +1;
		break;
	default:
		return false;
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
				// Get new vertex pos
				int32_t ax = testx + VtxX[i];
				int32_t ay = testy + VtxY[i];
				if (CheckTouchableMaterial(ax, ay, i))
				{
					found = false;
					break;
				}
				// Can attach here?
				if (CheckTouchableMaterial(ax + xcd, ay + ycd, i, ycd))
				{
					found = true;
					any_contact = true;
					// Store attachment material
					AttachMat = GBackMat(ax + xcd, ay + ycd);
					// Store absolute attachment position
					iAttachX = ax + xcd;
					iAttachY = ay + ycd;
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
	// Lines require at least 2 vertices
	if (VtxNum < 2)
	{
		return false;
	}

	// No modification
	if ((VtxX[cvtx] == tx) && (VtxY[cvtx] == ty))
	{
		return true;
	}

	// Check new path
	int32_t ix;
	int32_t iy;
	if (PathFree(tx, ty, VtxX[cvtx + ld], VtxY[cvtx + ld], &ix, &iy))
	{
		// Okay, set vertex
		VtxX[cvtx] = tx;
		VtxY[cvtx] = ty;
		return true;
	}
	else
	{
		// Intersected, find bend vertex
		bool found = false;
		int32_t cix;
		int32_t ciy;
		for (int irange = 4; irange <= 12; irange += 4)
		{
			for (cix = ix - irange / 2; cix <= ix + irange; cix += irange)
			{
				for (ciy = iy - irange / 2; ciy <= iy + irange; ciy += irange)
				{
					if (PathFree(cix, ciy, tx, ty) && PathFree(cix, ciy, VtxX[cvtx + ld], VtxY[cvtx + ld]))
					{
						found = true;
						goto out;
					}
				}
			}
		}
out:
		if (!found)
		{
			// Try bending directly at path the line took
			// Allow going through vehicle in this case to allow lines through castles and elevator shafts
			cix = oldx;
			ciy = oldy;
			if (!PathFreeIgnoreVehicle(cix, ciy, tx, ty) || !PathFreeIgnoreVehicle(cix, ciy, VtxX[cvtx + ld], VtxY[cvtx + ld]))
			{
				return false; // Found no bend vertex
			}
		}
		// Insert bend vertex
		if (ld > 0)
		{
			if (!InsertVertex(cvtx + 1, cix, ciy))
			{
				return false;
			}
		}
		else
		{
			if (!InsertVertex(cvtx, cix, ciy))
			{
				return false;
			}
			cvtx++;
		}
		// Okay, set vertex
		VtxX[cvtx] = tx;
		VtxY[cvtx] = ty;
		return true;
	}

	return false;
}

bool C4Shape::InsertVertex(int32_t index_position, int32_t tx, int32_t ty)
{
	if (VtxNum + 1 > C4D_MaxVertex)
	{
		return false;
	}
	if (!Inside<int32_t>(index_position, 0, VtxNum))
	{
		return false;
	}
	// Insert vertex before iPos
	for (int32_t i = VtxNum; i > index_position; i--)
	{
		VtxX[i] = VtxX[i - 1];
		VtxY[i] = VtxY[i - 1];
	}
	VtxX[index_position] = tx;
	VtxY[index_position] = ty;
	VtxNum++;
	return true;
}

bool C4Shape::RemoveVertex(int32_t index_position)
{
	if (!Inside<int32_t>(index_position, 0, VtxNum - 1))
	{
		return false;
	}
	for (int32_t i = index_position; i + 1 < VtxNum; i++)
	{
		VtxX[i] = VtxX[i + 1];
		VtxY[i] = VtxY[i + 1];
	}
	VtxNum--;
	return true;
}

bool C4Shape::CheckContact(int32_t at_x, int32_t at_y)
{
	// Check all vertices at given object position.
	// Return true on any contact.

	for (int32_t i = 0; i < VtxNum; i++)
	{
		if (!(VtxCNAT[i] & CNAT_NoCollision))
		{
			if (CheckTouchableMaterial(at_x + VtxX[i], at_y + VtxY[i], i))
			{
				return true;
			}
		}
	}

	return false;
}

bool C4Shape::ContactCheck(int32_t at_x, int32_t at_y, uint32_t *border_hack_contacts, bool collide_halfvehic)
{
	// Check all vertices at given object position.
	// Set ContactCNAT and ContactCount.
	// Set VtxContactCNAT and VtxContactMat.
	// Return true on any contact.

	ContactCNAT = CNAT_None;
	ContactCount = 0;

	for (int32_t vertex = 0; vertex < VtxNum; vertex++)
	{
		// Ignore vertex if collision has been flagged out
		if (!(VtxCNAT[vertex] & CNAT_NoCollision))
		{
			VtxContactCNAT[vertex] = CNAT_None;
			int32_t x = at_x + VtxX[vertex];
			int32_t y = at_y + VtxY[vertex];
			VtxContactMat[vertex] = GBackMat(x, y);

			if (CheckTouchableMaterial(x, y, vertex, collide_halfvehic? 1:0))
			{
				ContactCNAT |= VtxCNAT[vertex];
				VtxContactCNAT[vertex] |= CNAT_Center;
				ContactCount++;
				// Vertex center contact, now check top, bottom, left, right
				// Not using our style guideline here, is more readable in "table" format
				if (CheckTouchableMaterial(x, y - 1, vertex, collide_halfvehic ? 1 : 0)) VtxContactCNAT[vertex] |= CNAT_Top;
				if (CheckTouchableMaterial(x, y + 1, vertex, collide_halfvehic ? 1 : 0)) VtxContactCNAT[vertex] |= CNAT_Bottom;
				if (CheckTouchableMaterial(x - 1, y, vertex, collide_halfvehic ? 1 : 0)) VtxContactCNAT[vertex] |= CNAT_Left;
				if (CheckTouchableMaterial(x + 1, y, vertex, collide_halfvehic ? 1 : 0)) VtxContactCNAT[vertex] |= CNAT_Right;
			}
			if (border_hack_contacts)
			{
				if (x == 0 && CheckTouchableMaterial(x - 1, y, vertex))
				{
					*border_hack_contacts |= CNAT_Left;
				}
				else if (x == ::Landscape.GetWidth() && CheckTouchableMaterial(x + 1, y, vertex))
				{
					*border_hack_contacts |= CNAT_Right;
				}
			}
		}
	}


	return !!ContactCount;
}

bool C4Shape::CheckScaleToWalk(int x, int y)
{
	for (int32_t i = 0; i < VtxNum; i++)
	{
		if (VtxCNAT[i] & CNAT_NoCollision)
		{
			continue;
		}
		if (VtxCNAT[i] & CNAT_Bottom)
		{
			// No ground under the feet?
			if (CheckTouchableMaterial(x + VtxX[i], y + VtxY[i] + 1, i))
			{
				return false;
			}
		}
		else
		{
			// Can climb with hands?
			if (CheckTouchableMaterial(x + VtxX[i] - 1, y + VtxY[i], i))
			{
				return false;
			}
			if (CheckTouchableMaterial(x + VtxX[i] + 1, y + VtxY[i], i))
			{
				return false;
			}
		}
	}
	return true;
}

int32_t C4Shape::GetVertexX(int32_t iVertex)
{
	if (!Inside<int32_t>(iVertex, 0, VtxNum - 1))
	{
		return 0;
	}
	return VtxX[iVertex];
}

int32_t C4Shape::GetVertexY(int32_t iVertex)
{
	if (!Inside<int32_t>(iVertex, 0, VtxNum - 1))
	{
		return 0;
	}
	return VtxY[iVertex];
}

void C4Shape::CopyFrom(C4Shape source, bool copy_vertices, bool copy_vertices_from_self)
{
	if (copy_vertices)
	{
		// Truncate / copy vertex count
		VtxNum = (copy_vertices_from_self ? std::min<int32_t>(VtxNum, C4D_VertexCpyPos) : source.VtxNum);

		// Restore vertices from back of own buffer (retaining count)
		int32_t iCopyPos = (copy_vertices_from_self ? C4D_VertexCpyPos : 0);
		C4Shape &vertices_from = (copy_vertices_from_self ? *this : source);

		memcpy(VtxX,           vertices_from.VtxX + iCopyPos,           VtxNum * sizeof(*VtxX));
		memcpy(VtxY,           vertices_from.VtxY + iCopyPos,           VtxNum * sizeof(*VtxY));
		memcpy(VtxCNAT,        vertices_from.VtxCNAT + iCopyPos,        VtxNum * sizeof(*VtxCNAT));
		memcpy(VtxFriction,    vertices_from.VtxFriction + iCopyPos,    VtxNum * sizeof(*VtxFriction));
		memcpy(VtxContactCNAT, vertices_from.VtxContactCNAT + iCopyPos, VtxNum * sizeof(*VtxContactCNAT));
		memcpy(VtxContactMat,  vertices_from.VtxContactMat + iCopyPos,  VtxNum * sizeof(*VtxContactMat));
	}

	// Copies other members
	*((C4Rect *) this) = source;
	AttachMat = source.AttachMat;
	ContactCNAT = source.ContactCNAT;
	ContactCount = source.ContactCount;
}

int32_t C4Shape::GetBottomVertex()
{
	// Return bottom-most vertex
	int32_t iMax = -1;
	for (int32_t i = 0; i < VtxNum; i++)
	{
		if (VtxCNAT[i] & CNAT_Bottom)
		{
			if (iMax == -1 || VtxY[i] < VtxY[iMax])
			{
				iMax = i;
			}
		}
	}
	return iMax;
}

int C4Shape::GetBottom()
{
	int b = INT_MIN;
	for (int32_t i = 0; i < VtxNum; i++)
	{
		if (~VtxCNAT[i] & CNAT_NoCollision)
		{
			if (VtxY[i] > b)
			{
				b = VtxY[i];
			}
		}
	}
	if (b == INT_MIN)
	{
		return y + Hgt;
	}
	return b;
}

C4DensityProvider DefaultDensityProvider;

int32_t C4DensityProvider::GetDensity(int32_t x, int32_t y) const
{
	// Default density provider checks the landscape
	return GBackDensity(x, y);
}

int32_t C4Shape::GetVertexContact(int32_t iVertex, DWORD dwCheckMask, int32_t tx, int32_t ty, const C4DensityProvider &rDensityProvider)
{
	int32_t contact_bits = 0;

	// Range check
	if (!Inside<int32_t>(iVertex, 0, VtxNum - 1))
	{
		return contact_bits;
	}

	// Default check mask
	if (!dwCheckMask)
	{
		dwCheckMask = VtxCNAT[iVertex];
	}

	// Check vertex positions
	tx += VtxX[iVertex];
	ty += VtxY[iVertex];

	// Check all directions for solid material
	if (~VtxCNAT[iVertex] & CNAT_NoCollision)
	{
		// Not using our style guideline here, is more readable in "table" format
		if (dwCheckMask & CNAT_Center) if (CheckTouchableMaterial(tx, ty  , iVertex, 0, rDensityProvider)) contact_bits |= CNAT_Center;
		if (dwCheckMask & CNAT_Left)   if (CheckTouchableMaterial(tx-1, ty, iVertex, 0, rDensityProvider)) contact_bits |= CNAT_Left;
		if (dwCheckMask & CNAT_Right)  if (CheckTouchableMaterial(tx+1, ty, iVertex, 0, rDensityProvider)) contact_bits |= CNAT_Right;
		if (dwCheckMask & CNAT_Top)    if (CheckTouchableMaterial(tx, ty-1, iVertex, 0, rDensityProvider)) contact_bits |= CNAT_Top;
		if (dwCheckMask & CNAT_Bottom) if (CheckTouchableMaterial(tx, ty+1, iVertex, 1, rDensityProvider)) contact_bits |= CNAT_Bottom;
	}
	// Return resulting bitmask
	return contact_bits;
}

void C4Shape::CreateOwnOriginalCopy(C4Shape &source)
{
	// Copy vertices from original buffer, including count
	VtxNum = std::min<int32_t>(source.VtxNum, C4D_VertexCpyPos);
	memcpy(VtxX + C4D_VertexCpyPos,           source.VtxX,           VtxNum * sizeof(*VtxX));
	memcpy(VtxY + C4D_VertexCpyPos,           source.VtxY,           VtxNum * sizeof(*VtxY));
	memcpy(VtxCNAT + C4D_VertexCpyPos,        source.VtxCNAT,        VtxNum * sizeof(*VtxCNAT));
	memcpy(VtxFriction + C4D_VertexCpyPos,    source.VtxFriction,    VtxNum * sizeof(*VtxFriction));
	memcpy(VtxContactCNAT + C4D_VertexCpyPos, source.VtxContactCNAT, VtxNum * sizeof(*VtxContactCNAT));
	memcpy(VtxContactMat + C4D_VertexCpyPos,  source.VtxContactMat,  VtxNum * sizeof(*VtxContactMat));
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

	// A default shape is given in object compilation context only
	bool fRuntime = !!default_shape;
	C4Shape default_def_shape;
	if (!default_shape)
	{
		default_shape = &default_def_shape;
	}
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
	if (fRuntime)
	{
		pComp->Value(mkNamingAdapt( iAttachX,                   "AttachX",            0                 ));
		pComp->Value(mkNamingAdapt( iAttachY,                   "AttachY",            0                 ));
		pComp->Value(mkNamingAdapt( iAttachVtx,                 "AttachVtx",          0                 ));
	}
}
