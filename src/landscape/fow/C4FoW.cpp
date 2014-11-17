
#include "C4Include.h"
#include "C4FoW.h"

#include <float.h>


C4FoW::C4FoW()
	: pLights(NULL)
{
}

void C4FoW::Add(C4Object *pObj)
{
	// No view range? Probably want to remove instead
	if(!pObj->PlrViewRange)
	{
		Remove(pObj);
		return;
	}

	// Look for matching light
	C4FoWLight *pLight;
	for (pLight = pLights; pLight; pLight = pLight->getNext())
		if (pLight->getObj() == pObj)
			break;

	if (pLight)
	{

		// Update reach
		pLight->SetReach(pObj->PlrViewRange, 50);

	}
	else
	{
		// Create new light otherwise
		pLight = new C4FoWLight(pObj);
		pLight->pNext = pLights;
		pLights = pLight;
	}
}

void C4FoW::Remove(C4Object *pObj)
{
	// Look for matching light
	C4FoWLight *pPrev = NULL, *pLight;
	for (pLight = pLights; pLight; pPrev = pLight, pLight = pLight->getNext())
		if (pLight->getObj() == pObj)
			break;
	if (!pLight)
		return;

	// Remove
	(pPrev ? pLights : pPrev->pNext) = pLight->getNext();
	delete pLight;
}

void C4FoW::Invalidate(C4Rect r)
{
	for (C4FoWLight *pLight = pLights; pLight; pLight = pLight->getNext())
		pLight->Invalidate(r);
}

void C4FoW::Update(C4Rect r)
{
	for (C4FoWLight *pLight = pLights; pLight; pLight = pLight->getNext())
		pLight->Update(r);
}

void C4FoW::Render(C4FoWRegion *pRegion, const C4TargetFacet *pOnScreen)
{
	for (C4FoWLight *pLight = pLights; pLight; pLight = pLight->getNext())
		pLight->Render(pRegion, pOnScreen);
}
