#ifndef C4FOW_H
#define C4FOW_H

#include "StdBuf.h"

/** This class represents one ray. ...TODO*/
class C4FoWRay
{
public:
	C4FoWRay(int32_t iLeftX, int32_t iLeftY, int32_t iRightX, int32_t iRightY)
		: iLeftX(iLeftX), iLeftY(iLeftY), iRightX(iRightX), iRightY(iRightY),
		  iLeftEndY(0), iRightEndY(0),
		  iError(0),
		  fDirty(true),
		  pNext(NULL)
	{ }

private:
	int32_t iLeftX, iLeftY; // left delimiter point
	int32_t iRightX, iRightY; // right delimiter point
	int32_t iLeftEndY, iRightEndY; // where it hit solid material. C4FoWRayActive while currently being followed.
	int32_t iError; // How much error this ray has
	bool fDirty; // landscape changed since it was followed?
	C4FoWRay *pNext;

public:
	bool isDirty() const { return fDirty; }
	bool isClean() const { return !fDirty; }
	C4FoWRay *getNext() const { return pNext; }

	// Get a point on the ray boundary.
	inline int32_t getLeftX(int32_t y) const { return iLeftX * y / iLeftY; }
	inline int32_t getRightX(int32_t y) const { return iRightX * y / iRightY; }
	inline float getLeftXf(int32_t y) const { return (iLeftX * y) / float(iLeftY); }
	inline float getRightXf(int32_t y) const { return (iRightX * y) / float(iRightY); }
	// TODO: why is iLeftY or iRightY never 0?

	int32_t getLeftEndY() const { return iLeftEndY; }
	int32_t getLeftEndX() const { return getLeftX(iLeftEndY); }
	float getLeftEndXf() const { return getLeftXf(iLeftEndY); }
	int32_t getRightEndY() const { return iRightEndY; }
	int32_t getRightEndX() const { return getRightX(iRightEndY); }
	float getRightEndXf() const { return getRightXf(iRightEndY); }

	StdStrBuf getDesc() const;

	/* returns whether the given point is left of an imaginery line drawn from the left delimiter point to the origin */
	bool isLeft(int x, int y) const {
		return iLeftX * y > x * iLeftY;
	}
	
	/* returns whether the given point is right of an imaginery line drawn from the right delimiter point to the origin */
	bool isRight(int x, int y) const {
		return iRightX * y < x * iRightY;
	}
	
	/* returns whether the given point is inside a cone spanned by the ray between the left delimiter and right
	   delimiter point and the ray spanned from the origin to the left delimiter point.
	   
	   In case the origin is on the line with the two delimiter points, this function returns whether the point
	   is on the line spanned by the left and right delimiter points. */
	bool isInside(int x, int y) const {
		return !isLeft(x, y) && !isRight(x, y);
	}

	void SetLeft(int x, int y) { iLeftX = x; iLeftY = y; }
	void SetRight(int x, int y) { iRightX = x; iRightY = y; }

	bool MergeRight(int x, int y);
	bool MergeLeft(int x, int y);
	
	/* Split this ray into two: this ray and the returned one. The given point x,y is the position at
	   which the two resulting rays are connected with their left/right endpoints.
	   It is asserted that the given point is on the same line as the delimiting points. */
	C4FoWRay *Split(int x, int y);
	/* Remove the two following vertices and connect this ray to the right delimiter point of the next but one ray.
	   The position x,y is only used for the error calcualation and not actually inserted.
	   Returns false and does not do the action in case the error threshold would be reached with these parameters
	    */
	// ASK: iError is not added to the error counter of this ray on success
	// ASK: this removes TWO rays (but the comment says otherwise), why not one?
	// ASK: the x,y coordinate given is never actually added - even thout it counts towards the error counter
	bool Eliminate(int x, int y);
	
	void MergeDirty();
	
	void Clean(int32_t y);
	void Dirty(int32_t y);
	void Prune(int32_t y);

	
	// ASK: why are they called left/right delimiter and not start and endpoints?
	// ASK: left delimiter, right delimiter, origin always on one line? (if yes: add assertions)
	
	// TODO: find out more about this DIRTY stuff
};

#endif // C4FOWRAY_H