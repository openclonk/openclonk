
#ifndef C4FOW_H
#define C4FOW_H

#include "C4Surface.h"
#include "C4FacetEx.h"
#include "C4Rect.h"
#include "C4Object.h"
#include "C4FoWLight.h"
#include "C4FoWAmbient.h"
#include "C4Shader.h"

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
	C4FoWAmbient Ambient;

	// Shader to use for updating the frame buffer
	C4Shader *GetFramebufShader();

	void Clear();

	/** Updates the view range of the given object in its associated light or create a new light if none exists yet. */
	void Add(C4Object *pObj);
	/** Removes the light associated with the given object, if any */
	void Remove(C4Object *pObj);

	/** Update all light beams within the given rectangle */
	void Update(C4Rect r);
	/** Triggers the recalculation of all light beams within the given rectangle because the landscape changed. */
	void Invalidate(C4Rect r);

	void Render(class C4FoWRegion *pRegion, const C4TargetFacet *pOnScreen = NULL);

private:
	// Shader for updating the frame buffer
	C4Shader FramebufShader;
};

#endif // C4FOW_H
