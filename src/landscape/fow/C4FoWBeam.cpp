/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2014-2016, The OpenClonk Team and contributors
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

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"

#ifndef USE_CONSOLE
#include "landscape/fow/C4FoWBeam.h"

// Maximum error allowed while merging beams.
const int32_t C4FoWMergeThreshold = 10; // (in landscape pixels * 2)

// A = 1/2 | a x b |
static inline int32_t getDoubleTriangleSurface(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3)
{
	int32_t ax = x2 - x1, ay = y2 - y1;
	int32_t bx = x3 - x1, by = y3 - y1;
	// We don't bother to actually halve so we can stay with integers.
	// Doesn't matter as long as we keep in mind the threshold needs to
	// be doubled.
	return abs(ax * by - ay * bx);
}

StdStrBuf C4FoWBeam::getDesc() const {
	return FormatString("%d:%d@%d:%d%s",
		getLeftX(1000),
		getRightX(1000),
		getLeftEndY(),
		getRightEndY(),
		fDirty ? "*" : "");
}

bool C4FoWBeam::MergeRight(int32_t x, int32_t y)
{
	// Note: Right-merging is the most common and most important optimization.
	// This procedure will probably be *hammered* as a result. Worth inlining?

	assert(!isDirty()); assert(isRight(x, y));

	// Calculate error. Note that simply summing up errors is not correct,
	// strictly speaking (as new and old error surfaces might overlap). Still,
	// this is quite elaborate already, no need to make it even more
	int32_t iErr = getDoubleTriangleSurface(
		getLeftEndX(), iLeftEndY,
		getRightEndX(), iRightEndY,
		x, y);
	if (iError + iErr > C4FoWMergeThreshold)
		return false;

	// Move right endpoint.
	iRightX = x;
	iRightY = y;
	iRightEndY = y;
	iError += iErr;
	return true;
}

bool C4FoWBeam::MergeLeft(int32_t x, int32_t y)
{
	assert(!isDirty()); assert(isLeft(x, y));

	// Calculate error.
	float iErr = getDoubleTriangleSurface(
		getLeftEndX(), iLeftEndY,
		getRightEndX(), iRightEndY,
		x, y);
	if (iError + iErr > C4FoWMergeThreshold)
		return false;

	// Move left endpoint.
	iLeftX = x;
	iLeftY = y;
	iLeftEndY = y;
	iError += iErr;
	return true;
}

bool C4FoWBeam::EliminateRight(int32_t x, int32_t y)
{
	// Called on the beams left of the one getting eliminated
	C4FoWBeam *pElim = pNext, *pMerge = pNext->pNext;
	assert(!!pElim); assert(!!pMerge);
	assert(!isDirty()); assert(!pMerge->isDirty());

	// Calc errors, add those accumulated on both merged beams
	int32_t iErr = getDoubleTriangleSurface(
		getLeftEndX(), getLeftEndY(),
		pMerge->getRightEndX(), pMerge->getRightEndY(),
		x, y);
	iErr += pMerge->iError;
	if (iError + iErr > C4FoWMergeThreshold)
		return false;

	// Do elimination
	iRightX = pMerge->iRightX;
	iRightY = pMerge->iRightY;
	iRightEndY = pMerge->iRightEndY;
	pNext = pMerge->pNext;
	iError += iErr;
	delete pElim;
	delete pMerge;
	return true;
}

C4FoWBeam *C4FoWBeam::Split(int32_t x, int32_t y)
{
	// Make sure to never create negative-surface beams
	assert(isDirty()); assert(isInside(x, y));

	// Allocate a new beam. Ugh, expensive.
	C4FoWBeam *pBeam = new C4FoWBeam(x, y, iRightX, iRightY);
	pBeam->Dirty(iLeftEndY);

	// Move to make space
	iRightX = x;
	iRightY = y;

	// Relink
	pBeam->pNext = pNext;
	pNext = pBeam;
	return pBeam;
}

void C4FoWBeam::MergeDirty()
{
	// As a rule, dirty beams following each other should
	// always be merged, so splits can be reverted once
	// the landscape changes.
	C4FoWBeam *pWith = pNext;
	assert(isDirty()); assert(!!pWith); assert(pWith->isDirty());

	// Figure out how far the new dirty beams reaches. Note that
	// we might lose information about the landscape here.
	Dirty(std::min(getLeftEndY(), pWith->getLeftEndY()));

	// Set right
	iRightX = pWith->iRightX;
	iRightY = pWith->iRightY;

	// Relink & delete
	pNext = pWith->getNext();
	delete pWith;
}

void C4FoWBeam::Clean(int32_t y)
{
	// Search hit something, this beam is now clean.
	iLeftEndY = y;
	iRightEndY = y;
	fDirty = false;
}

void C4FoWBeam::Dirty(int32_t y)
{
	// Got invalidated, beam is dirty until updated
	iLeftEndY = y;
	iRightEndY = y;
	fDirty = true;
}

void C4FoWBeam::Prune(int32_t y)
{
	// Check which sides we need to prune
	bool fLeft = (iLeftEndY >= y),
		 fRight = (iRightEndY >= y);
	// If both sides got pruned, we are clean
	// (can't possibly extend this beam further)
	if (fLeft && fRight)
		Clean(y);
	else if (fLeft)
		iLeftEndY = y;
	if (fRight)
		iRightEndY = y;
}

void C4FoWBeam::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(iLeftX, "iLeftX"));
	pComp->Value(mkNamingAdapt(iLeftY, "iLeftY"));
	pComp->Value(mkNamingAdapt(iRightX, "iRightX"));
	pComp->Value(mkNamingAdapt(iRightY, "iRightY"));
	pComp->Value(mkNamingAdapt(iLeftEndY, "iLeftEndY"));
	pComp->Value(mkNamingAdapt(iRightEndY, "iRightEndY"));
	pComp->Value(mkNamingAdapt(iError, "iError"));
	pComp->Value(mkNamingAdapt(fDirty, "fDirty"));
}

#endif
