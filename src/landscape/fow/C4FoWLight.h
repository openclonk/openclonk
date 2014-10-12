#ifndef C4FOWLIGHT_H
#define C4FOWLIGHT_H

#include "C4Object.h"
#include "C4Rect.h"
#include "C4Surface.h"
#include "C4FacetEx.h"

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
	class C4FoWLightSection *pSections;
	C4FoWLight *pNext;
	C4Object *pObj; // Associated object

public:
	int32_t getX() const { return iX; }
	int32_t getY() const { return iY; }
	int32_t getReach() const { return iReach; }
	int32_t getFadeout() const { return iFadeout; }
	// ASK: the code suggests taht total reach is iReach and iFadeout is subtracted from it, rather than added
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

	void Render(class C4FoWRegion *pRegion, const C4TargetFacet *pOnScreen = NULL);

};

#endif