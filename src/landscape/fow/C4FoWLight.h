#ifndef C4FOWLIGHT_H
#define C4FOWLIGHT_H

#include "C4Object.h"
#include "C4Surface.h"
#include "C4FacetEx.h"
#include "C4FoWLightSection.h"
#include "C4Rect.h"

#include <vector>

/** This class represents one light source. A light source has an associated object with which the light source moves
    and one light section that handles the light beams for each direction (up, down, left, right).

    Furthermore, each light source has a size. This is usually the object's PlrViewRange. See SetReach.
*/
class C4FoWLight
{
	friend class C4FoW;
public:
	C4FoWLight(C4Object *pObj);
	~C4FoWLight();

private:
	int32_t iX, iY; // center position
	int32_t iReach; // maximum length of beams
	int32_t iFadeout; // number of pixels over which beams fade out
	int32_t iSize; // size of the light source. Decides smoothness of shadows
	C4FoWLight *pNext;
	C4Object *pObj; // Associated object

	std::vector<C4FoWLightSection*> sections;

public:
	int32_t getX() const { return iX; }
	int32_t getY() const { return iY; }
	int32_t getReach() const { return iReach; }
	int32_t getFadeout() const { return iFadeout; }
	int32_t getTotalReach() const { return iReach + iFadeout; }
	int32_t getSize() const { return iSize; }
	C4FoWLight *getNext() const { return pNext; }
	C4Object *getObj() const { return pObj; }

	/** Sets the light's size in pixels. The reach is the total radius of the light while the fadeout is the number of 
	    pixels after which the light should dim down */
	void SetReach(int32_t iReach, int32_t iFadeout);
	
	/** Triggers the recalculation of all light beams within the given rectangle for this light because the landscape changed. */
	void Invalidate(C4Rect r);
	/** Update all light beams within the given rectangle for this light */
	void Update(C4Rect r);
	/** Render this light*/
	void Render(class C4FoWRegion *pRegion, const C4TargetFacet *pOnScreen = NULL);

private:
	/** Calculate the intermediate fade points used for constructing the intermediate fade triangles later on */
	void CalculateIntermediateFadeTriangles(std::list<class C4FoWBeamTriangle> &triangles) const;

	void ProjectPointOutward(float &x, float &y, float maxDistance) const;

	/** Draws the triangle fan (the area with 100% light around the light source) with the given strategy */
	void DrawFan(class C4FoWDrawStrategy* pen, std::list<class C4FoWBeamTriangle> &triangles) const;
	/** Draws the fadeoot triangles - those around the triangle fan - with the given strategy */
	void DrawFade(C4FoWDrawStrategy* pen, std::list<C4FoWBeamTriangle> &triangles) const;
	/** Draws the fadeout triangles in between the normal fadeout triangles with the given strategy */
	void DrawIntermediateFadeTriangles(C4FoWDrawStrategy* pen, std::list<C4FoWBeamTriangle> &triangles) const;
	/** Returns the (squared) distance from this light source to the given point. Squared simply because we only need this
	    for comparison of distances. So we don't bother to sqrt it */
	float GetSquaredDistanceTo(int32_t x, int32_t y) const { return (x - getX()) * (x - getX()) + (y - getY()) * (y - getY()); }

};

/** Finds the point where the line through (ax,ay) and (bx,by) crosses through the line
    through (px,py) and (qx,qy). Returns false if the lines are parallel, even if they
	are on the same line. The intersection point it put into (ix,iy). Optionally, if 
	specified, the abParameter is the parameter torecalculate the point on the line between
	a and b: a + abParameter * (b-a) = (ix,iy). */
bool find_cross(float ax, float ay, float bx, float by,
                float px, float py, float qx, float qy,
				float *ix, float *iy, float *abParameter = NULL);

#endif