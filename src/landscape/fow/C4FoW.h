
#ifndef C4FOW_H
#define C4FOW_H

#include "C4Surface.h"
#include "C4FacetEx.h"
#include "C4Rect.h"
#include "C4Object.h"
#include "C4FoWLight.h"

/**
	This class holds all lights for the objects. It forwards the update, invalidation and render calls each to the
	lights.
*/
class C4FoW
{
public:
	C4FoW();

private:
	/** linked list of all lights */
	class C4FoWLight *pLights;

public:
	void Clear();

	/** Updates the view range of the given object in its associated light or create a new light if none exists yet. */
	void Add(C4Object *pObj);
	/** Removes the light associated with the given object, if any */
	void Remove(C4Object *pObj);

	/** Update all light rays within the given rectangle */
	// ASK: only called by C4FoWRegion??
	void Update(C4Rect r);
	/** Triggers the recalculation of all light rays within the given rectangle because the landscape changed. */
	void Invalidate(C4Rect r);

	void Render(class C4FoWRegion *pRegion, const C4TargetFacet *pOnScreen = NULL);
};

#endif // C4FOW_H