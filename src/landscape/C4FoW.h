
#ifndef C4FOW_H
#define C4FOW_H

#include "C4Rect.h"
#include "C4Surface.h"

class C4FoW
{
public:
	C4FoW();

private:
	class C4FoWLight *pLights;

public:
	void Add(C4Object *pObj);
	void Remove(C4Object *pObj);
	void Update(C4Rect r);
	void Invalidate(C4Rect r);

	void Render(class C4FoWRegion *pRegion, const C4TargetFacet *pOnScreen = NULL);
};

class C4FoWRegion
{
public:
	C4FoWRegion(C4FoW *pFoW);
	~C4FoWRegion();

private:
	C4FoW *pFoW;
	C4Rect Region, OldRegion;
	C4Surface *pSurface, *pBackSurface;
	GLuint hFrameBufDraw, hFrameBufRead;

public:
	const C4Rect &getRegion() const { return Region; }
	const C4Surface *getSurface() const { return pSurface; }
	const C4Surface *getBackSurface() const { return pBackSurface; }

	bool Create();
	void Clear();

	void Set(C4Rect r) { Region = r; }
	void Render(const C4TargetFacet *pOnScreen = NULL);

};

class C4FoWLight
{
	friend class C4FoW;
public:
	C4FoWLight(C4Object *pObj);
	~C4FoWLight();

private:
	int32_t iX, iY; // center position
	int32_t iReach; // maximum length of rays
	int32_t iFadeout; // number of pixels over which rays fade out
	int32_t iSize; // size of the light source. Decides smoothness of shadows
	class C4FoWLightSection *pSections;
	C4FoWLight *pNext;
	C4Object *pObj; // Associated object

public:
	int32_t getX() const { return iX; }
	int32_t getY() const { return iY; }
	int32_t getReach() const { return iReach; }
	int32_t getFadeout() const { return iFadeout; }
	int32_t getTotalReach() const { return iReach + iFadeout; }
	int32_t getSize() const { return iSize; }
	C4FoWLight *getNext() const { return pNext; }
	C4Object *getObj() const { return pObj; }

	void SetReach(int32_t iReach, int32_t iFadeout);

	void Invalidate(C4Rect r);
	void Update(C4Rect r);

	void Render(C4FoWRegion *pRegion, const C4TargetFacet *pOnScreen = NULL);

};

class C4FoWLightSection
{
public:
	C4FoWLightSection(C4FoWLight *pLight, int r, C4FoWLightSection *pNext = NULL);
	~C4FoWLightSection();

private:

	// Center light
	C4FoWLight *pLight;

	// Transformation matrix
	int iRot;
	int a, b, c, d;
	int ra, rb, rc, rd;

	// Rays
	class C4FoWRay *pRays;

	// List
	C4FoWLightSection *pNext;

public:

	C4FoWLightSection *getNext() const { return pNext; }

	void Invalidate(C4Rect r);
	void Update(C4Rect r);

	void Render(C4FoWRegion *pRegion, const C4TargetFacet *pOnScreen = NULL);

	void Prune(int32_t iReach);
	void Dirty(int32_t iReach);
	
	void ClearRays();

private:

	// Ray to landscape. Ray coordinates are with light source at (0,0).
	template <class T> T transDX(T dx, T dy) const { return T(a) * dx + T(b) * dy; }
	template <class T> T transDY(T dx, T dy) const { return T(c) * dx + T(d) * dy; }
	template <class T> T transX(T x, T y) const { return transDX(x, y) + T(pLight->getX()); }
	template <class T> T transY(T x, T y) const { return transDY(x, y) + T(pLight->getY()); }

	// Landscape to ray
	template <class T> T rtransDX(T dx, T dy) const { return T(ra) * dx + T(rb) * dy; }
	template <class T> T rtransDY(T dx, T dy) const { return T(rc) * dx + T(rd) * dy; }
	template <class T> T rtransX(T x, T y) const { return rtransDX(x-T(pLight->getX()),y-T(pLight->getY())); }
	template <class T> T rtransY(T x, T y) const { return rtransDY(x-T(pLight->getX()),y-T(pLight->getY())); }

	C4Rect rtransRect(C4Rect r) const {
		C4Rect Rect(rtransX(r.x, r.y), rtransY(r.x, r.y),
		            rtransDX(r.Wdt, r.Hgt), rtransDY(r.Wdt, r.Hgt));
		Rect.Normalize();
		return Rect;
	}

	bool isConsistent() const;

	int32_t RectLeftMostY(const C4Rect &r) const { return r.x >= 0 ? r.y+r.Hgt : r.y; }
	int32_t RectRightMostY(const C4Rect &r) const { return r.x + r.Wdt <= 0 ? r.y+r.Hgt : r.y; }

	C4FoWRay *FindRayLeftOf(int32_t x, int32_t y); // find right-most ray left of point
	C4FoWRay *FindRayOver(int32_t x, int32_t y); // find left-most ray to extend over point

};

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
	inline float getLeftXf(int32_t y) const { return float(iLeftX * y) / float(iLeftY); }
	inline float getRightXf(int32_t y) const { return float(iRightX * y) / float(iRightY); }

	int32_t getLeftEndY() const { return iLeftEndY; }
	int32_t getLeftEndX() const { return getLeftX(iLeftEndY); }
	float getLeftEndXf() const { return getLeftXf(iLeftEndY); }
	int32_t getRightEndY() const { return iRightEndY; }
	int32_t getRightEndX() const { return getRightX(iRightEndY); }
	float getRightEndXf() const { return getRightXf(iRightEndY); }

	StdStrBuf getDesc() const;

	bool isLeft(int x, int y) const {
		return iLeftX * y > x * iLeftY;
	}
	bool isRight(int x, int y) const {
		return iRightX * y < x * iRightY;
	}
	bool isInside(int x, int y) const {
		return !isLeft(x, y) && !isRight(x, y);
	}

	void SetLeft(int x, int y) { iLeftX = x; iLeftY = y; }
	void SetRight(int x, int y) { iRightX = x; iRightY = y; }

	bool MergeRight(int x, int y);
	bool MergeLeft(int x, int y);
	bool Eliminate(int x, int y);
	C4FoWRay *Split(int x, int y);
	void MergeDirty();
	
	void Clean(int32_t y);
	void Dirty(int32_t y);
	void Prune(int32_t y);

};

#endif // C4FOW_H