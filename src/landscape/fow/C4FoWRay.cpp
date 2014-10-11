
#include "C4Include.h"
#include "C4FoWRay.h"

// Maximum error allowed while merging rays.
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

StdStrBuf C4FoWRay::getDesc() const {
	return FormatString("%d:%d@%d:%d%s",
		getLeftX(1000),
		getRightX(1000),
		getLeftEndY(),
		getRightEndY(),
		fDirty ? "*" : "");
}

bool C4FoWRay::MergeRight(int x, int y)
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

bool C4FoWRay::MergeLeft(int x, int y)
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

bool C4FoWRay::Eliminate(int x, int y)
{
	// Called on the ray left of the one getting eliminated
	C4FoWRay *pElim = pNext, *pMerge = pNext->pNext;
	assert(!!pElim); assert(!!pMerge);
	assert(!isDirty()); assert(!pMerge->isDirty());

	// Calc errors, add those accumulated on both merged rays
	int32_t iErr = getDoubleTriangleSurface(
		getLeftEndX(), iLeftEndY,
		pMerge->getRightEndX(), pMerge->iLeftEndY,
		x, y);
	iErr += iError + pMerge->iError;
	if (iErr > C4FoWMergeThreshold)
		return false;

	// Do elimination
	iRightX = pMerge->iRightX;
	iRightY = pMerge->iRightY;
	iRightEndY = pMerge->iRightEndY;
	pNext = pMerge->pNext;
	delete pElim;
	delete pMerge;
	return true;
}

C4FoWRay *C4FoWRay::Split(int x, int y)
{
	// Make sure to never create negative-surface rays
	assert(isDirty()); assert(isInside(x, y));

	// Allocate a new ray. Ugh, expensive.
	C4FoWRay *pRay = new C4FoWRay(x, y, iRightX, iRightY);
	pRay->Dirty(iLeftEndY);

	// Move to make space
	iRightX = x;
	iRightY = y;

	// Relink
	pRay->pNext = pNext;
	pNext = pRay;
	return pRay;
}

void C4FoWRay::MergeDirty()
{
	// As a rule, dirty rays following each other should
	// always be merged, so splits can be reverted once
	// the landscape changes.
	C4FoWRay *pWith = pNext;
	assert(isDirty()); assert(!!pWith); assert(pWith->isDirty());

	// Figure out how far the new dirty ray reaches. Note that
	// we might lose information about the landscape here.
	Dirty(Min(getLeftEndY(), pWith->getLeftEndY()));

	// Set right
	iRightX = pWith->iRightX;
	iRightY = pWith->iRightY;

	// Relink & delete
	pNext = pWith->getNext();
	delete pWith;
}

void C4FoWRay::Clean(int y)
{
	// Search hit something, this ray is now clean.
	assert(isDirty());
	iLeftEndY = y;
	iRightEndY = y;
	fDirty = false;
}

void C4FoWRay::Dirty(int y)
{
	// Got invalidated, ray is dirty until updated
	iLeftEndY = y;
	iRightEndY = y;
	fDirty = true;
}

void C4FoWRay::Prune(int32_t y)
{
	// Check which sides we need to prune
	bool fLeft = (iLeftEndY >= y),
		 fRight = (iRightEndY >= y);
	// If both sides got pruned, we are clean
	// (can't possibly extend this ray further)
	if (fLeft && fRight)
		Clean(y);
	else if (fLeft)
		iLeftEndY = y;
	if (fRight)
		iRightEndY = y;
}