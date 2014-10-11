
#include "C4Include.h"
#include "C4FoWLight.h"
#include "C4FoWLightSection.h"

C4FoWLight::C4FoWLight(C4Object *pObj)
	: iX(fixtoi(pObj->fix_x)), iY(fixtoi(pObj->fix_y)),
	  iReach(pObj->PlrViewRange), iFadeout(50), iSize(20),
	  pNext(NULL), pObj(pObj)
{
	pSections = new C4FoWLightSection(this, 0);
	pSections = new C4FoWLightSection(this, 90, pSections);
	pSections = new C4FoWLightSection(this, 180, pSections);
	pSections = new C4FoWLightSection(this, 270, pSections);
}

C4FoWLight::~C4FoWLight()
{
	while (C4FoWLightSection *pSect = pSections) {
		pSections = pSect->getNext();
		delete pSect;
	}
}

void C4FoWLight::Invalidate(C4Rect r)
{
	for (C4FoWLightSection *pSect = pSections; pSect; pSect = pSect->getNext())
		pSect->Invalidate(r);
}


void C4FoWLight::SetReach(int32_t iReach2, int32_t iFadeout2)
{
	// Fadeout changes don't matter
	iFadeout = iFadeout2;

	if (iReach == iReach2) return;

	// Reach decreased? Prune rays
	if (iReach2 < iReach) {
		iReach = iReach2;
		for (C4FoWLightSection *pSect = pSections; pSect; pSect = pSect->getNext())
			pSect->Prune(iReach);

	} else {

		// Reach increased? Dirty rays that might get longer now
		iReach = iReach2;
		for (C4FoWLightSection *pSect = pSections; pSect; pSect = pSect->getNext())
			pSect->Dirty(iReach);
	}
}

void C4FoWLight::Update(C4Rect Rec)
{

	// Update position from object. Clear if we moved in any way
	int32_t iNX = fixtoi(pObj->fix_x), iNY = fixtoi(pObj->fix_y);
	if (iNX != iX || iNY != iY)
	{
		for (C4FoWLightSection *pSect = pSections; pSect; pSect = pSect->getNext())
			// pruning to zero length results in the rays being cleared and new ones created
			pSect->Prune(0);
		iX = iNX; iY = iNY;
	}

	for (C4FoWLightSection *pSect = pSections; pSect; pSect = pSect->getNext())
		pSect->Update(Rec);

}

void C4FoWLight::Render(C4FoWRegion *pRegion, const C4TargetFacet *pOnScreen)
{
	for (C4FoWLightSection *pSect = pSections; pSect; pSect = pSect->getNext())
		pSect->Render(pRegion, pOnScreen);
}

