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

#ifndef C4FOWBEAM_H
#define C4FOWBEAM_H

#include "C4ForbidLibraryCompilation.h"

#ifndef USE_CONSOLE
#include "lib/StdBuf.h"

/** This class represents one beam. A beam is a triangle spanned by two rays: one going from the origin to the
    left delimiter point, one going from the origin to the right delimiter point.

	LeftEndX/Y and RightEndX/Y are the positions where this beam hit a solid material pixel while LeftX/Y and RightX/Y
	are the positions at which this beam did not hit but merely streaked a solid pixel (= the neighboring beam hits it)
	and thus continues until *EndX/Y.

	It is assumed that the beam always goes down in display coordinate system, thus the Y-position of the delimiting
	points is always > 0. C4FoWLightSection transforms the coordinate system after calculation to implement the beams
	that go into other directions. This class here always assumes one direction.

	A beam is initially marked as "dirty", meaning that the end points of the beam haven't been found yet (by the ray
	trace algorithm) and the beam will extend further. When all beams are "clean", the algorithm is done.
	*/
class C4FoWBeam
{
public:
	C4FoWBeam(int32_t iLeftX, int32_t iLeftY, int32_t iRightX, int32_t iRightY)
		: iLeftX(iLeftX), iLeftY(iLeftY), iRightX(iRightX), iRightY(iRightY),
		  iLeftEndY(0), iRightEndY(0),
		  iError(0),
		  fDirty(true),
		  pNext(nullptr)
	{ }

private:
	int32_t iLeftX, iLeftY; // left delimiter point
	int32_t iRightX, iRightY; // right delimiter point
	int32_t iLeftEndY, iRightEndY; // where it hit solid material.
	int32_t iError; // How much error this beam has
	bool fDirty; // landscape changed since it was followed?
	C4FoWBeam *pNext;

public:
	bool isDirty() const { return fDirty; }
	bool isClean() const { return !fDirty; }
	C4FoWBeam *getNext() const { return pNext; }
	void setNext(C4FoWBeam *next) { pNext=next; }

	// Get a point on the beam boundary.
	inline int32_t getLeftX(int32_t y) const { return iLeftX * y / iLeftY; }
	inline int32_t getRightX(int32_t y) const { return iRightX * y / iRightY; }
	inline float getLeftXf(int32_t y) const { return (iLeftX * y) / float(iLeftY); }
	inline float getRightXf(int32_t y) const { return (iRightX * y) / float(iRightY); }

	int32_t getLeftEndY() const { return iLeftEndY; }
	int32_t getLeftEndX() const { return getLeftX(iLeftEndY); }
	float getLeftEndXf() const { return getLeftXf(iLeftEndY); }
	int32_t getRightEndY() const { return iRightEndY; }
	int32_t getRightEndX() const { return getRightX(iRightEndY); }
	float getRightEndXf() const { return getRightXf(iRightEndY); }

	StdStrBuf getDesc() const;

	/** returns whether the given point is left of an imaginery line drawn from the origin to the left delimiter point (point is left of beam) */
	bool isLeft(int32_t x, int32_t y) const {
		return iLeftX * y > x * iLeftY;
	}
	
	/** returns whether the given point is right of an imaginery line drawn from the origin to the right delimiter point (point is right of beam) */
	bool isRight(int32_t x, int32_t y) const {
		return iRightX * y < x * iRightY;
	}
	
	/** returns whether the given point is inside the triangle that is defined by this beam */
	bool isInside(int32_t x, int32_t y) const {
		return !isLeft(x, y) && !isRight(x, y);
	}

	void SetLeft(int32_t x, int32_t y) { iLeftX = x; iLeftY = y; }
	void SetRight(int32_t x, int32_t y) { iRightX = x; iRightY = y; }

	/** Set the new right delimiter point to the given point if the resulting difference in size is less than the error
	    threshold. If successfull, adds the introduced error to iError.

	    Asserts that the given point is really right of the right delimiter point
	*/
	bool MergeRight(int32_t x, int32_t y);

	/** Set the new left delimiter point to the given point if the resulting difference in size is less than the error
	    threshold. If successfull, adds the introduced error to iError.

	    Asserts that the given point is really left of the right delimiter point
	*/
	bool MergeLeft(int32_t x, int32_t y);
	
	/** Split this beam into two: this beam and the returned one. The given point x,y is the position at
	    which the two resulting beams are connected with their left/right endpoints.
	    It is asserted that the given point is inside the previous beam. (So the beam can only made smaller) */
	C4FoWBeam *Split(int32_t x, int32_t y);
	/** Makes this beam span from its left delimiter point to the right delimiter point of the next but one beam,
	    removing the two beams in between in the process.
	    In other words, removes the next beam and merges this beam with the next but one beam.
	    Returns false and does not do the action in case the error threshold would be reached.
	    */
	bool EliminateRight(int32_t x, int32_t y);
	
	void MergeDirty();
	
	/** Set a new end point, making the beam "clean". */
	void Clean(int32_t y);

	/** Extend this beam to the new maximum length, making it "dirty" because now the end points haven't been found anymore
	    Called when the size of the light has been increased to the given value. */
	void Dirty(int32_t y);

	/** Prune this beam to a new maximum length.
	    Called when the size of the light has been decreased to the given value. */
	void Prune(int32_t y);

	// Serialization for debugging
	void CompileFunc(StdCompiler *pComp);

};

#endif

#endif // C4FOWBEAM
