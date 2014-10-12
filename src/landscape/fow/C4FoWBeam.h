#ifndef C4FOWBEAM_H
#define C4FOWBEAM_H

#include "StdBuf.h"

/** This class represents one beam. A beam is a triangle spanned by two line segment: one going from the origin to the
    left delimiter point, one going from the origin to the right delimiter point.

	Other than this, a beam keeps the position at which it hit solid material, each for the left and for the right
	line segment.

	A beam can be marked as dirty. This means... TODO
	*/
class C4FoWBeam
{
public:
	C4FoWBeam(int32_t iLeftX, int32_t iLeftY, int32_t iRightX, int32_t iRightY)
		: iLeftX(iLeftX), iLeftY(iLeftY), iRightX(iRightX), iRightY(iRightY),
		  iLeftEndY(0), iRightEndY(0),
		  iError(0),
		  fDirty(true),
		  pNext(NULL)
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

	/** returns whether the given point is left of an imaginery line drawn from the left delimiter point to the origin (point is left of beam) */
	bool isLeft(int x, int y) const {
		return iLeftX * y > x * iLeftY;
	}
	
	/** returns whether the given point is right of an imaginery line drawn from the right delimiter point to the origin (point is right of beam) */
	bool isRight(int x, int y) const {
		return iRightX * y < x * iRightY;
	}
	
	/** returns whether the given point is inside the triangle that is defined by this beam */
	bool isInside(int x, int y) const {
		return !isLeft(x, y) && !isRight(x, y);
	}

	void SetLeft(int x, int y) { iLeftX = x; iLeftY = y; }
	void SetRight(int x, int y) { iRightX = x; iRightY = y; }

	/** Set the new right delimiter point to the given point if the resulting difference in size is less than the error
	    threshold. If successfull, adds the introduced error to iError.

	    Asserts that the given point is really right of the right delimiter point
	*/
	// ASK: this procedure must certainly involve the beam right of this one, the code for this is probably in C4FoWLightSection? If yes, this should probably be managed by this class? (as it already manages things like that in Split and Eliminate)
	bool MergeRight(int x, int y);

	/** Set the new left delimiter point to the given point if the resulting difference in size is less than the error
	    threshold. If successfull, adds the introduced error to iError.

	    Asserts that the given point is really left of the right delimiter point
	*/
	bool MergeLeft(int x, int y);
	
	/** Split this beam into two: this beam and the returned one. The given point x,y is the position at
	    which the two resulting beams are connected with their left/right endpoints.
	    It is asserted that the given point is inside the previous beam. (So the beam can only made smaller) */
	// ASK: is it intended that the beam can only be made smaller by splitting?
	C4FoWBeam *Split(int x, int y);
	/** Makes this beam span from its left delimiter point to the right delimiter point of the next but one beam,
	    removing the two beams in between in the process.
	    The position x,y is only used for the error calcualation and not actually inserted.
	    Returns false and does not do the action in case the error threshold would be reached with these parameters
	    */
	// ASK: iError is not added to the error counter of this beam on success
	// ASK: the x,y coordinate given is never actually added - even though it counts towards the error counter
	bool Eliminate(int x, int y);
	
	void MergeDirty();
	
	void Clean(int32_t y);
	void Dirty(int32_t y);
	void Prune(int32_t y);
	
	// TODO: find out more about this DIRTY stuff
};

#endif // C4FOWBEAM