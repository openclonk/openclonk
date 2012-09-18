
#include "C4Include.h"
#include "C4FoW.h"
#include "C4Rect.h"
#include "C4Landscape.h"
#include "C4DrawGL.h"
#include "C4Object.h"

#include <float.h>

// TODO: Make sure to use int32_t throughout!

//#define LIGHT_DEBUG

bool glCheck() {
	if (int err = glGetError()) {
		LogF("GL error %d: %s", err, gluErrorString(err));
		return false;
	}
	return true;
}

// Maximum error allowed while merging rays. Actually double, see below.
const int32_t C4FoWMergeThreshold = 5; // (in landscape pixels)

// A = 1/2 | a x b |
static inline int32_t getTriangleSurface(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3)
{
	int32_t ax = x2 - x1, ay = y2 - y1;
	int32_t bx = x3 - x1, by = y3 - y1;
	// We don't bother to actually halve so we can stay with integers.
	// Doesn't matter as long as we keep in mind the threshold needs to
	// be doubled.
	return abs(ax * by - ay * bx);
}

C4FoW::C4FoW()
	: pLights(NULL)
{
}

void C4FoW::Add(C4Object *pObj)
{
	// No view range? Probably want to remove instead
	if(!pObj->PlrViewRange) {
		Remove(pObj);
		return;
	}

	// Look for matching light
	C4FoWLight *pLight;
	for (pLight = pLights; pLight; pLight = pLight->getNext())
		if (pLight->getObj() == pObj)
			break;
	if (pLight) {

		// Update reach
		pLight->SetReach(pObj->PlrViewRange, 50);

	} else {

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
	// Invalidate all lights
	for (C4FoWLight *pLight = pLights; pLight; pLight = pLight->getNext())
		pLight->Invalidate(r);
}

void C4FoWLight::Invalidate(C4Rect r)
{
	// Invalidate all sections
	for (C4FoWLightSection *pSect = pSections; pSect; pSect = pSect->getNext())
		pSect->Invalidate(r);
}

void C4FoW::Update(C4Rect r)
{
	// Update all lights
	for (C4FoWLight *pLight = pLights; pLight; pLight = pLight->getNext())
		pLight->Update(r);
}

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

C4FoWLightSection::C4FoWLightSection(C4FoWLight *pLight, int r, C4FoWLightSection *pNext)
	: pLight(pLight), iRot(r), pNext(pNext)
{
	// Rotation matrices
	iRot = r % 360;
	switch(iRot % 360) {
	default:
		assert(false);
	case 0:
		a = 1; b = 0; c = 0; d = 1;
		ra = 1; rb = 0; rc = 0; rd = 1;
		break;
	case 90:
		a = 0; b = 1; c = -1; d = 0;
		ra = 0; rb = -1; rc = 1; rd = 0;
		break;
	case 180:
		a = -1; b = 0; c = 0; d = -1;
		ra = -1; rb = 0; rc = 0; rd = -1;
		break;
	case 270: 
		a = 0; b = -1; c =1; d = 0;
		ra = 0; rb = 1; rc = -1; rd = 0;
		break;
	}
	// Ray list
	pRays = new C4FoWRay(-1, +1, +1, +1);
}

C4FoWLight::~C4FoWLight()
{
	while (C4FoWLightSection *pSect = pSections) {
		pSections = pSect->getNext();
		delete pSect;
	}
}

C4FoWLightSection::~C4FoWLightSection()
{
	ClearRays();
}

void C4FoWLightSection::ClearRays()
{
	while (C4FoWRay *pRay = pRays) {
		pRays = pRay->getNext();
		delete pRay;
	}
}

void C4FoWLight::SetReach(int32_t iReach2, int32_t iFadeout2)
{
	// Fadeout changes don't matter
	iFadeout = iFadeout2;

	// Reach unchanged? Easy.
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

void C4FoWLightSection::Prune(int32_t iReach)
{
	if (iReach == 0) {
		ClearRays();
		pRays = new C4FoWRay(-1, 1, 1, 1);
		return;
	}
	// TODO: Merge active rays that we have pruned to same length
	for (C4FoWRay *pRay = pRays; pRay; pRay = pRay->getNext())
		pRay->Prune(iReach);
}

void C4FoWLightSection::Dirty(int32_t iReach)
{
	for (C4FoWRay *pRay = pRays; pRay; pRay = pRay->getNext())
		if (pRay->getLeftEndY() >= iReach || pRay->getRightEndY() >= iReach)
			pRay->Dirty(Min(pRay->getLeftEndY(), pRay->getRightEndY()));
}

C4FoWRay *C4FoWLightSection::FindRayLeftOf(int32_t x, int32_t y)
{
	// Trivial
	y = Max(y, 0);
	if (!pRays || !pRays->isRight(x, y))
		return NULL;
	// Go through list
	// Note: In case this turns out expensive, one might think about implementing
	// a skip-list. But I highly doubt it.
	C4FoWRay *pRay = pRays;
	while (pRay->getNext() && pRay->getNext()->isRight(x, y))
		pRay = pRay->getNext();
	return pRay;
}

C4FoWRay *C4FoWLightSection::FindRayOver(int32_t x, int32_t y)
{
	C4FoWRay *pPrev = FindRayLeftOf(x, y);
	return pPrev ? pPrev->getNext() : pRays;
}

void C4FoWLight::Update(C4Rect Rec)
{

	// Update position from object. Clear if we moved in any way
	int32_t iNX = fixtoi(pObj->fix_x), iNY = fixtoi(pObj->fix_y);
	if (iNX != iX || iNY != iY) {
		for (C4FoWLightSection *pSect = pSections; pSect; pSect = pSect->getNext())
			pSect->Prune(0);
		iX = iNX; iY = iNY;
	}

	// Update all sections
	for (C4FoWLightSection *pSect = pSections; pSect; pSect = pSect->getNext())
		pSect->Update(Rec);

}

void C4FoWLightSection::Update(C4Rect RectIn)
{

	// Transform rectangle into our coordinate system
	C4Rect Rect = rtransRect(RectIn);
	C4Rect Bounds = rtransRect(C4Rect(0,0,GBackWdt,GBackHgt));

#ifdef LIGHT_DEBUG
	if (!::Game.iTick255) {
		LogSilentF("Full ray list:");
		StdStrBuf Rays;
		for(C4FoWRay *pRay = pRays; pRay; pRay = pRay->getNext()) {
			Rays.AppendChar(' ');
			Rays.Append(pRay->getDesc());
		}
		LogSilent(Rays.getData());
	}
#endif

#ifdef LIGHT_DEBUG
	LogSilentF("Update %d/%d-%d/%d", Rect.x, Rect.y, Rect.x+Rect.Wdt, Rect.y+Rect.Hgt);
#endif

	// Out of reach?
	if (Rect.y > pLight->getTotalReach())
		return;
	
	// Get last ray that's positively *not* affected
	int iLY = Max(0, RectLeftMostY(Rect)),
		iRX = Rect.x+Rect.Wdt, iRY = Max(0, RectRightMostY(Rect));
	C4FoWRay *pStart = FindRayLeftOf(Rect.x, iLY);

	// Skip clean rays
	while (C4FoWRay *pNext = pStart ? pStart->getNext() : pRays) {
		if (pNext->isDirty()) break;
		pStart = pNext;
	}
	// Find end ray, determine at which position we have to start scanning
	C4FoWRay *pRay = pStart ? pStart->getNext() : pRays;
#ifdef LIGHT_DEBUG
	if (pRay)
		LogSilentF("Start ray is %s", pRay->getDesc().getData());
#endif
	C4FoWRay *pEnd = NULL;
	int32_t iStartY = Rect.GetBottom();
	while (pRay && !pRay->isLeft(Rect.x+Rect.Wdt, iRY)) {
		if (pRay->isDirty() && pRay->getLeftEndY() <= Rect.y+Rect.Hgt) {
			pEnd = pRay;
			iStartY = Min(iStartY, pRay->getLeftEndY());
		}
		pRay = pRay->getNext();
	}

	// Can skip scan completely?
	if (!pEnd)
		return;

	// Update right end coordinates
#ifdef LIGHT_DEBUG
	LogSilentF("End ray is %s", pEnd->getDesc().getData());
#endif

	if (pEnd->isRight(iRX, iRY)) {
		iRX = pEnd->getRightEndX() + 1;
		iRY = pEnd->getRightEndY();
		// We want pEnd itself to get scanned
		//assert(!pEnd->isLeft(iRX, iRY));
	}
#ifdef LIGHT_DEBUG
	//LogSilentF("Right limit at %d, start line %d", 1000 * (iRX - iX) / (iRY - iY), iStartY);
#endif

	// Bottom of scan - either bound by rectangle or by light's reach
	int32_t iEndY = Min(Rect.GetBottom(), pLight->getReach());

	// Scan lines
	int32_t y;
	for(y = Max(0, iStartY); y < iEndY; y++) {

		// Scan all rays
		C4FoWRay *pLast = pStart; int32_t iDirty = 0;
		for(C4FoWRay *pRay = pStart ? pStart->getNext() : pRays; pRay; pLast = pRay, pRay = pRay->getNext()) {
			assert(pLast ? pLast->getNext() == pRay : pRay == pRays);

			// Clean (enough)?
			if (!pRay->isDirty() || y < pRay->getLeftEndY())
				continue;

			// Out left?
			if (pRay->isRight(Rect.x, y))
				continue;
			// Out right?
			if (pRay->isLeft(Rect.x + Rect.Wdt, y) || pRay->isLeft(iRX, iRY))
				break;

			// We have an active ray that we're about to scan
			iDirty++;
			pRay->Dirty(y+1);

			// Do a scan
			int32_t xl = Max(pRay->getLeftX(y), Bounds.x),
			        xr = Min(pRay->getRightX(y), Bounds.x+Bounds.Wdt-1);
			for(int x = xl; x <= xr; x++) {

				// Fast free?
				if (!Landscape._FastSolidCheck(transX(x,y), transY(x,y)))
				{
					if (a == 1)
						x = rtransX(Landscape.FastSolidCheckNextX(transX(x,y)) - 1, 0);
					continue;
				}

				// Free?
				if (!GBackSolid(transX(x,y), transY(x,y))) continue;

				// Split points
				int x1 = x - 1, x2 = x + 1;
				bool fSplitLeft = !pRay->isLeft(x1, y);
				bool fSplitRight = !pRay->isRight(x2, y);

				// Double merge?
				if (!fSplitLeft && !fSplitRight && pLast && pRay->getNext()) {
					if(pLast->Eliminate(x, y)) {
						pRay = pLast;
						break; // no typo. fSplitRight => x == xr
					}
				}

				// Merge possible?
				if (!fSplitLeft && fSplitRight && pLast)
					if (pLast->MergeRight(x2, y)) {
						pRay->SetLeft(x2, y);
						assert(pRay->isDirty());
						continue;
					}
				if (fSplitLeft && !fSplitRight && pRay->getNext())
					if (pRay->getNext()->MergeLeft(x1, y)) {
						pRay->SetRight(x1, y);
						break; // no typo. fSplitRight => x == xr
					}

				// Split out left
				if (fSplitLeft) {
					pLast = pRay;
					pRay = pLast->Split(x1, y);
					assert(pLast->getNext() == pRay);
				}

				// Split out right
				if(fSplitRight) {
					pLast = pRay;
					pRay = pLast->Split(x2, y);
					assert(pLast->getNext() == pRay);

					// Deactivate left/middle ray
					pLast->Clean(y);
					assert(pRay->isDirty());

				} else {

					// Deactivate ray
					pRay->Clean(y);
					break;

				}

			}

		}

		// No active rays left?
		if (!iDirty)
			break;
			
	}

	// At end of light's reach? Mark all rays that got scanned all the way to the end as clean.
	// There's no need to scan them anymore.
	if (y >= pLight->getReach()) {
		for (C4FoWRay *pRay = pStart ? pStart->getNext() : pRays; pRay; pRay = pRay->getNext())
			if (pRay->isDirty() && pRay->getLeftEndY() > pLight->getReach())
				pRay->Clean(pLight->getReach());
	}

#ifdef LIGHT_DEBUG
	LogSilentF("Updated ray list:");
	for(C4FoWRay *pRay = pStart ? pStart->getNext() : pRays; pRay; pRay = pRay->getNext()) {
		if (pRay->isLeft(iRX, iRY))
			break;
		LogSilent(pRay->getDesc().getData());
	}
#endif
}

void C4FoWLightSection::Invalidate(C4Rect r)
{
	// Assume normalized rectangle
	assert(r.Wdt > 0 && r.Hgt > 0);
	
	// Get rectangle corners that bound the possibly affected rays
	int iLY = RectLeftMostY(r), iRY = RectRightMostY(r);
	C4FoWRay *pLast = FindRayLeftOf(r.x, iLY);
	C4FoWRay *pRay = pLast ? pLast->getNext() : pRays;

	// Scan over rays
	while (pRay && !pRay->isLeft(r.x+r.Wdt, iRY)) {

		// Dirty ray?
		if (pRay->getLeftEndY() > r.y || pRay->getRightEndY() > r.y)
			pRay->Dirty(r.y);

		// Merge with last ray?
		if (pLast && pLast->isDirty() && pRay->isDirty()) {
			pLast->MergeDirty();
			pRay = pLast->getNext();

		// Advance otherwise
		} else {
			pLast = pRay;
			pRay = pRay->getNext();
		}
	}

	// Final check for merging dirty rays on the right end
	if (pLast && pRay && pLast->isDirty() && pRay->isDirty())
		pLast->MergeDirty();

}

bool C4FoWRay::MergeRight(int x, int y)
{
	// Note: Right-merging is the most common and most important optimization.
	// This procedure will probably be *hammered* as a result. Worth inlining?

	assert(!isDirty()); assert(isRight(x, y));

	// Calculate error. Note that simply summing up errors is not correct,
	// strictly speaking (as new and old error surfaces might overlap). Still,
	// this is quite elaborate already, no need to make it even more 
	int32_t iErr = getTriangleSurface(
		getLeftEndX(), iLeftEndY,
		getRightEndX(), iRightEndY,
		x, y);
	if (iError + iErr > C4FoWMergeThreshold)
		return false;

	// Move right endpoint.
	iRightX = x;
	iRightY = y;
	iRightEndY = y;
	iError += iErr;
	return true;
}

bool C4FoWRay::MergeLeft(int x, int y)
{
	assert(!isDirty()); assert(isLeft(x, y));

	// Calculate error.
	float iErr = getTriangleSurface(
		getLeftEndX(), iLeftEndY,
		getRightEndX(), iRightEndY,
		x, y);
	if (iError + iErr > C4FoWMergeThreshold)
		return false;

	// Move left endpoint.
	iLeftX = x;
	iLeftY = y;
	iLeftEndY = y;
	iError += iErr;
	return true;
}

bool C4FoWRay::Eliminate(int x, int y)
{
	// Called on the ray left of the one getting eliminated
	C4FoWRay *pElim = pNext, *pMerge = pNext->pNext;
	assert(!!pElim); assert(!!pMerge);
	assert(!isDirty()); assert(!pMerge->isDirty());

	// Calc errors, add those accumulated on both merged rays
	int32_t iErr = getTriangleSurface(
		getLeftEndX(), iLeftEndY,
		pMerge->getRightEndX(), pMerge->iLeftEndY,
		x, y);
	iErr += iError + pMerge->iError;
	if (iErr > C4FoWMergeThreshold)
		return false;

	// Do elimination
	iRightX = pMerge->iRightX;
	iRightY = pMerge->iRightY;
	iRightEndY = pMerge->iRightEndY;
	pNext = pMerge->pNext;
	delete pElim;
	delete pMerge;
	return true;
}

C4FoWRay *C4FoWRay::Split(int x, int y)
{
	// Make sure to newer create negative-surface rays
	assert(isDirty()); assert(isInside(x, y));

	// Allocate a new ray. Ugh, expensive.
	C4FoWRay *pRay = new C4FoWRay(x, y, iRightX, iRightY);
	pRay->Dirty(iLeftEndY);

	// Move to make space
	iRightX = x;
	iRightY = y;

	// Relink
	pRay->pNext = pNext;
	pNext = pRay;
	return pRay;
}

void C4FoWRay::MergeDirty()
{
	// As a rule, dirty rays following each other should
	// always be merged, so splits can be reverted once
	// the landscape changes.
	C4FoWRay *pWith = pNext;
	assert(isDirty()); assert(!!pWith); assert(pWith->isDirty());

	// Figure out how far the new dirty ray reaches. Note that
	// we might lose information about the landscape here.
	Dirty(Min(getLeftEndY(), pWith->getLeftEndY()));

	// Set right
	iRightX = pWith->iRightX;
	iRightY = pWith->iRightY;

	// Relink & delete
	pNext = pWith->getNext();
	delete pWith;
}

void C4FoWRay::Clean(int y)
{
	// Search hit something, this ray is now clean.
	assert(isDirty());
	iLeftEndY = y;
	iRightEndY = y;
	fDirty = false;
}

void C4FoWRay::Dirty(int y)
{
	// Got invalidated, ray is dirty until updated
	iLeftEndY = y;
	iRightEndY = y;
	fDirty = true;
}

void C4FoWRay::Prune(int32_t y)
{
	// Check which sides we need to prune
	bool fLeft = (iLeftEndY >= y),
		 fRight = (iRightEndY >= y);
	// If both sides got pruned, we are clean
	// (can't possibly extend this ray further)
	if (fLeft && fRight)
		Clean(y);
	else if (fLeft)
		iLeftEndY = y;
	if (fRight)
		iRightEndY = y;
}

C4FoWRegion::C4FoWRegion(C4FoW *pFoW, C4Player *pPlayer)
	: pFoW(pFoW)
	, pPlayer(pPlayer)
	, hFrameBufDraw(0), hFrameBufRead(0)
	, Region(0,0,0,0), OldRegion(0,0,0,0)
	, pSurface(NULL), pBackSurface(NULL)
{
}

C4FoWRegion::~C4FoWRegion()
{
	Clear();
}

bool C4FoWRegion::BindFramebuf()
{

	// Flip texture
	C4Surface *pSfc = pSurface;
	pSurface = pBackSurface;
	pBackSurface = pSfc;

	// Can simply reuse old texture?
	if (!pSurface || pSurface->Wdt < Region.Wdt || pSurface->Hgt < Region.Hgt)
	{
		// Create texture
		if (!pSurface)
			pSurface = new C4Surface();
		if (!pSurface->Create(Region.Wdt, Region.Hgt))
			return false;
	}

	// Generate frame buffer object
	if (!hFrameBufDraw) {
		glGenFramebuffersEXT(1, &hFrameBufDraw);
		glGenFramebuffersEXT(1, &hFrameBufRead);
	}

	// Bind current texture to frame buffer
	glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, hFrameBufDraw);
	glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, hFrameBufRead);
	glFramebufferTexture2DEXT(GL_DRAW_FRAMEBUFFER_EXT,
		GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D,
		pSurface->ppTex[0]->texName, 0);
	if (pBackSurface)
		glFramebufferTexture2DEXT(GL_READ_FRAMEBUFFER_EXT,
			GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D,
			pBackSurface->ppTex[0]->texName, 0);

	// Check status, unbind if something was amiss
	GLenum status1 = glCheckFramebufferStatusEXT(GL_READ_FRAMEBUFFER_EXT),
		   status2 = glCheckFramebufferStatusEXT(GL_DRAW_FRAMEBUFFER_EXT);
	if (status1 != GL_FRAMEBUFFER_COMPLETE_EXT ||
		(pBackSurface && status2 != GL_FRAMEBUFFER_COMPLETE_EXT) ||
		!glCheck())
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		return false;
	}

	// Worked!
	return true;
}

void C4FoWRegion::Clear()
{
	if (hFrameBufDraw) {
		glDeleteFramebuffersEXT(1, &hFrameBufDraw);
		glDeleteFramebuffersEXT(1, &hFrameBufRead);
	}
	hFrameBufDraw = hFrameBufRead = 0;
	delete pSurface; pSurface = NULL;
	delete pBackSurface; pBackSurface = NULL;
}

void C4FoWRegion::Update(C4Rect r)
{
	// Set the new region
	Region = r;
}

void C4FoWRegion::Render(const C4TargetFacet *pOnScreen)
{
	// Update FoW at interesting location
	pFoW->Update(Region);

	// On screen? No need to set up frame buffer - simply shortcut
	if (pOnScreen)
	{
		pFoW->Render(this, pOnScreen);
		return;
	}

	// Create & bind the frame buffer
	pDraw->StorePrimaryClipper();
	if(!BindFramebuf())
	{
		pDraw->RestorePrimaryClipper();
		return;
	}
	assert(pSurface && hFrameBufDraw);
	if (!pSurface || !hFrameBufDraw)
		return;

	// Set up a clean context
	glViewport(0, 0, getSurface()->Wdt, getSurface()->Hgt);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, getSurface()->Wdt, getSurface()->Hgt, 0);

	// Clear texture contents
	glClearColor(0.0f, 0.5f/1.5f, 0.5f/1.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// Copy over the old state
	if (OldRegion.Wdt > 0) {

		int dx0 = Region.x - OldRegion.x,
			dy0 = Region.y - OldRegion.y,
			dx1 = Region.x + Region.Wdt - OldRegion.x - OldRegion.Wdt,
			dy1 = Region.y + Region.Hgt - OldRegion.y - OldRegion.Hgt;

		glBlitFramebufferEXT(
			Max(0, dx0),                  Max(0, -dy1),
			OldRegion.Wdt - Max(0, -dx1), OldRegion.Hgt - Max(0, dy0),
			Max(0, -dx0),                 Max(0, dy1),
			Region.Wdt - Max(0, dx1),     Region.Hgt - Max(0, -dy0),
			GL_COLOR_BUFFER_BIT, GL_LINEAR);

		glCheck();

		// Fade out. Note we constantly vary the alpha factor all the time -
		// this is barely visible but makes it a lot less likely that we 
		// hit cases where we add the same thing every time, but still don't
		// converge to the same color due to rounding.
		int iAdd = (Game.FrameCounter/3) % 2;
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(0.0f, 0.5f/1.5f, 0.5f/1.5f, 1.0f/16.0f+iAdd*1.0f/256.0f);
		glBegin(GL_QUADS);
		glVertex2i(0, 0);
		glVertex2i(getSurface()->Wdt, 0);
		glVertex2i(getSurface()->Wdt, getSurface()->Hgt);
		glVertex2i(0, getSurface()->Hgt);
		glEnd();
	}

	// Render FoW to frame buffer object
	glBlendFunc(GL_ONE, GL_ONE);
	pFoW->Render(this);
	
	// Done!
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	pDraw->RestorePrimaryClipper();
	glCheck();

	OldRegion = Region;

}

void C4FoW::Render(C4FoWRegion *pRegion, const C4TargetFacet *pOnScreen)
{
	// Render all lights
	for (C4FoWLight *pLight = pLights; pLight; pLight = pLight->getNext())
		pLight->Render(pRegion, pOnScreen);
}

void C4FoWLight::Render(C4FoWRegion *pRegion, const C4TargetFacet *pOnScreen)
{
	// Render all sections
	//C4FoWLightSection *pSect = pSections;
	for (C4FoWLightSection *pSect = pSections; pSect; pSect = pSect->getNext())
		pSect->Render(pRegion, pOnScreen);
}

// Gives the point where the line through (x1,y1) and (x2,y2) crosses through the line
// through (x3,y3) and (x4,y4)
bool find_cross(float x1, float y1, float x2, float y2,
                float x3, float y3, float x4, float y4,
				float *px, float *py, float *pb = NULL)
{
	// We are looking for a, b so that
	//  px = a*x1 + (1-a)*x2 = b*x3 + (1-b)*x4
	//  py = a*y1 + (1-a)*y2 = b*y3 + (1-b)*y4

	// Cross product
	float d = (x3-x4)*(y1-y2) - (y3-y4)*(x1-x2);
	if (d == 0) return false; // parallel - or vector(s) 0

	// We actually just need b - the unique solution
	// to above equation. A refreshing piece of elementary math
	// that I got wrong two times.
	float b = ((y4-y2)*(x1-x2) - (x4-x2)*(y1-y2)) / d;
	*px = b*x3 + (1-b)*x4;
	*py = b*y3 + (1-b)*y4;
	if (pb) *pb = b;

	// Sanity-test solution
#ifdef _DEBUG
	if (x1 != x2) {
		float a = (b*(x3-x4) + (x4-x2)) / (x1-x2);
		float eps = 0.01f;
		//assert(fabs(a*x1 + (1-a)*x2 - *px) < eps);
		//assert(fabs(a*y1 + (1-a)*y2 - *py) < eps);
	}
#endif

	return true;
}

void C4FoWLightSection::Render(C4FoWRegion *pRegion, const C4TargetFacet *pOnScreen)
{
	C4Rect Reg = rtransRect(pRegion->getRegion());

	// Find start ray
	int iLY = Max(0, RectLeftMostY(Reg)),
		iRY = Max(0, RectRightMostY(Reg)),
	    iRX = Reg.x + Reg.Wdt;
	C4FoWRay *pStart = FindRayOver(Reg.x, iLY);

	// Find end ray - determine the number of rays we actually need to draw
	C4FoWRay *pRay = pStart; int32_t iRayCnt = 0;
	while (pRay && !pRay->isLeft(iRX, iRY)) {

		// TODO: Remove clipped rays on the left so we can in the
		// most extreme case completely ignore lights here?

		pRay = pRay->getNext();
		iRayCnt++;
	}
	int32_t iOriginalRayCnt = iRayCnt;

	// Allocate arrays for our points (lots of them)
	float *gFanLX = new float [iRayCnt * 10],
		  *gFanLY = gFanLX + iRayCnt,
		  *gFanRX = gFanLY + iRayCnt,
		  *gFanRY = gFanRX + iRayCnt,
		  *gFadeLX = gFanRY + iRayCnt,
		  *gFadeLY = gFadeLX + iRayCnt,
		  *gFadeRX = gFadeLY + iRayCnt,
		  *gFadeRY = gFadeRX + iRayCnt,
		  *gFadeIX = gFadeRY + iRayCnt,
		  *gFadeIY = gFadeIX + iRayCnt;
	int32_t i;
	for (i = 0, pRay = pStart; i < iRayCnt; i++, pRay = pRay->getNext()) {
		gFanLX[i] = pRay->getLeftEndXf();
		gFanLY[i] = float(pRay->getLeftEndY());
		gFanRX[i] = pRay->getRightEndXf();
		gFanRY[i] = float(pRay->getRightEndY());
	}

	// Outputs the rightmost (l=1) or leftmost (l=-1) position of the
	// light, as seen from the given point. Shinks the light if it is
	// too close, to work against exessive fades.
#define ROUND_LIGHT
#ifdef ROUND_LIGHT
	const float gLightShrink = 5.0f;
	#define CALC_LIGHT(x, y, l, gOutX, gOutY) \
		float gOutX, gOutY; \
		{ \
			float d = sqrt(x * x + y * y); \
			float s = Min(float(pLight->getSize()), d / gLightShrink); \
			gOutX = s / d * l * y; \
			gOutY = s / d * l * -x; \
		}
	// Sadly, this causes rays to cross when the light shrinks...
#else
	// I think this is the only version that guarantees no crossing rays.
	// Unsatisfying as it might be.
	const float gLightShrink = 1.1f;
	#define CALC_LIGHT(x, y, l, gOutX, gOutY) \
		float gOutX, gOutY; \
		{ \
			float d = y; \
			gOutX = BoundBy(x + l * d * gLightShrink, -float(pLight->getSize()), +float(pLight->getSize())); \
			gOutY = 0; \
		}
#endif

	// Phase 1: Project lower point so it lies on a line with outer left/right
	// light lines.
	float gScanLevel = 0;
	for (int iStep = 0; iStep < 100000; iStep++) {

		// Find the ray to project. This makes this whole alrogithm O(n²),
		// but I see no other way to make the whole thing robust :/
		float gBestLevel = FLT_MAX;
		int j;
		for (j = 0; j+1 < iRayCnt; j++) {
			float gLevel = Min(gFanRX[j], gFanLX[j+1]);
			if (gLevel <= gScanLevel || gLevel >= gBestLevel)
				continue;
			gBestLevel = gLevel;
		}
		if(gBestLevel == FLT_MAX)
			break;
		gScanLevel = gBestLevel;

		for(int i = 0; i+1 < iRayCnt; i++) {

		if(Min(gFanRX[i], gFanLX[i+1]) != gBestLevel)
			continue;

		// Debugging
//#define FAN_STEP_DEBUG
#ifdef FAN_STEP_DEBUG
		LogSilentF("Fan step %d (i=%d)", iStep, i);
		for (j = 0; j < iRayCnt; j++) {
			LogSilentF(" %.02f %.02f", gFanLX[j], gFanLY[j]);
			LogSilentF(" %.02f %.02f", gFanRX[j], gFanRY[j]);	
		}
#endif

		// Calculate light bounds. We assume a "smaller" light for closer rays
		CALC_LIGHT(gFanRX[i], gFanRY[i], -1, gLightLX, gLightLY);
		CALC_LIGHT(gFanLX[i+1], gFanLY[i+1], 1, gLightRX, gLightRY);

		// Ascending
		float gCrossX, gCrossY;
		bool fDescendCollision = false;
		if (gFanRY[i] > gFanLY[i+1]) {


			// Left ray surface self-shadowing? We test whether the scalar product
			// of the ray's normal and the light vector is positive.
			if (  (gFanRY[i] - gFanLY[i]) * (gFanLX[i+1] - gLightRX) >=
				  (gFanRX[i] - gFanLX[i]) * (gFanLY[i+1] - gLightRY)) {

				// Reduce to upper point (Yep, we now that the upper point
				// must be the right one. Try to figure out why!)
				assert(gFanRY[i] <= gFanLY[i]);
				gFanLX[i] = gFanRX[i];
				gFanLY[i] = gFanRY[i];
			}

			// Left ray reduced?
			float gFanRXp = gFanRX[i]; float thresh = 1.0;
			if (gFanRX[i] == gFanLX[i] && gFanRY[i] == gFanLY[i]) {

				// Move point to the right for the purpose of finding the cross
				// point - after all, given that gFanRX[i] == gFanLX[i], we
				// only care about whether to eliminate or insert an additional
				// point for the descend collision, so the exact value doesn't
				// really matter - but we don't want find_cross to bail out!
				gFanRXp += 1.0;
				thresh = 0.0;
			}

			// Move right point of left ray to the left (the original point is partly shadowed)
			bool fEliminate = false; float b;
			bool f = find_cross(gLightRX, gLightRY, gFanLX[i+1], gFanLY[i+1],
			                    gFanLX[i], gFanLY[i], gFanRXp, gFanRY[i],
			                    &gCrossX, &gCrossY, &b);
			
			// The self-shadow-check should have made sure that the two are
			// never parallel.
			assert(f);

			// Cross point to left of surface? Then the surface itself is
			// shadowed, and we don't need to draw it.
			if (b >= thresh) {

				fEliminate = true;

			// Cross point actually right of surface? This can happen when
			// we eliminated surfaces. It means that the light doesn't reach
			// down far enough between this and the next ray to hit anything.
			// As a result, we insert a new zero-width surface where the light
			// stops.
			} else if (b < 0.0) {

				// As it doesn't matter from this point on whether we were
				// in an ascend or a descend case, this gets handled top-level.
				// ... still debating with myself whether a "goto" would be
				// cleaner here ;)
				fDescendCollision = true;

			} else {

				// Set cross point
				gFanRX[i] = gCrossX;
				gFanRY[i] = gCrossY;

			}

			// This shouldn't change the case we are in (uh, I think)
			assert(gFanRY[i] > gFanLY[i+1]);

			// Did we eliminate the surface with this step?
			if (fEliminate && i) {

				// Remove it then
				for(int j = i; j+1 < iRayCnt; j++) {
					gFanLX[j] = gFanLX[j+1];
					gFanLY[j] = gFanLY[j+1];
					gFanRX[j] = gFanRX[j+1];
					gFanRY[j] = gFanRY[j+1];
				}

				// With the elimination, we need to re-process the last
				// ray, as it might be more shadowed than we realized.
				// Note that the last point might have been projected already - 
				// but that's okay, 
				iRayCnt--; i-=2;
			}

		// Descending - same, but mirrored. And without comments.
		} else if (gFanRY[i] < gFanLY[i+1]) {
			if (  (gFanRY[i+1] - gFanLY[i+1]) * (gFanRX[i] - gLightLX) >=
				  (gFanRX[i+1] - gFanLX[i+1]) * (gFanRY[i] - gLightLY)) {
				assert(gFanLY[i+1] <= gFanRY[i+1]);
				gFanRX[i+1] = gFanLX[i+1];
				gFanRY[i+1] = gFanLY[i+1];
			}
			float gFanRXp = gFanRX[i+1], thresh = 0.0;
			if (gFanRX[i+1] == gFanLX[i+1] && gFanRY[i+1] == gFanLY[i+1]) {
				gFanRXp += 1.0;
				thresh = 1.0;
			}
			bool fEliminate = false;
			float b;
			bool f = find_cross(gLightLX, gLightLY, gFanRX[i], gFanRY[i],
			                    gFanLX[i+1], gFanLY[i+1], gFanRXp, gFanRY[i+1],
						        &gCrossX, &gCrossY, &b);
			assert(f);
			if (b <= thresh) {
				fEliminate = true;
			} else if (b > 1.0) {
				fDescendCollision = true;
			} else {
				gFanLX[i+1] = gCrossX;
				gFanLY[i+1] = gCrossY;
			}
			assert(gFanRY[i] < gFanLY[i+1]);
			if (fEliminate && i+2 < iRayCnt) {
				for(int j = i+1; j+1 < iRayCnt; j++) {
					gFanLX[j] = gFanLX[j+1];
					gFanLY[j] = gFanLY[j+1];
					gFanRX[j] = gFanRX[j+1];
					gFanRY[j] = gFanRY[j+1];
				}
				iRayCnt--; i--;
			}
		}

		if (fDescendCollision) {

			// Should never be parallel -- otherwise we wouldn't be here
			// in the first place.
			bool f = find_cross(gLightLX, gLightLY, gFanRX[i], gFanRY[i],
					            gLightRX, gLightRY, gFanLX[i+1], gFanLY[i+1],
							    &gCrossX, &gCrossY);
			assert(f);

			// The point should always be lower than both existing points
			assert(gCrossY >= gFanRY[i]);
			assert(gCrossY >= gFanLY[i+1]);

			// This should always follow an elimination, but better check
			assert(iOriginalRayCnt > iRayCnt);
			for (int j = iRayCnt - 1; j >= i+1; j--) {
				gFanLX[j+1] = gFanLX[j];
				gFanLY[j+1] = gFanLY[j];
				gFanRX[j+1] = gFanRX[j];
				gFanRY[j+1] = gFanRY[j];
			}

			// Okay, now i+1 should be free
			gFanLX[i+1] = gCrossX;
			gFanLY[i+1] = gCrossY;
			gFanRX[i+1] = gCrossX;
			gFanRY[i+1] = gCrossY;

			// Jump over surface. Note that our right ray might get
			// eliminated later on, causing us to back-track into this
			// zero-length pseudo-surface. This will cause find_cross
			// above to eliminate the pseudo-surface and back-track
			// further to the left, which is exactly how it should work.
			iRayCnt++; i++;
		}

		}
	}

	// Phase 2: Calculate fade points
#ifdef FAN_STEP_DEBUG
	LogSilent("Fade points");
#endif // FAN_STEP_DEBUG
	for (i = 0; i < iRayCnt; i++) {

		// Calculate light bounds. Note that the way light size is calculated
		// and we are using it below, we need to consider an "asymetrical" light.
		CALC_LIGHT(gFanRX[i], gFanRY[i], 1, gLightRX, gLightRY);
		CALC_LIGHT(gFanLX[i], gFanLY[i], -1, gLightLX, gLightLY);

		// This is simply the projection of the left point using the left-most
		// light point, as well as the projection of the right point using the
		// right-most light point.

		// For once we actually calculate this using the real distance
		float dx = gFanLX[i] - gLightLX, dy = gFanLY[i] - gLightLY;
		float d = float(pLight->getFadeout()) / sqrt(dx*dx + dy*dy);;
		gFadeLX[i] = gFanLX[i] + d * dx;
		gFadeLY[i] = gFanLY[i] + d * dy;

		dx = gFanRX[i] - gLightRX;
		dy = gFanRY[i] - gLightRY;
		d = float(pLight->getFadeout()) / sqrt(dx*dx + dy*dy);
		gFadeRX[i] = gFanRX[i] + d * dx;
		gFadeRY[i] = gFanRY[i] + d * dy;

		// Do the fades cross?
		if ((gFadeRX[i] - gLightRX) / (gFadeRY[i] - gLightRY)
			< (gFadeLX[i] - gLightRX) / (gFadeLY[i] - gLightRY)) {

			// Average it
			gFadeLX[i] = gFadeRX[i] = (gFadeLX[i] + gFadeRX[i]) / 2;
			gFadeLY[i] = gFadeRY[i] = (gFadeLY[i] + gFadeRY[i]) / 2;

		}

#ifdef FAN_STEP_DEBUG
		LogSilentF(" %.02f %.02f", gFadeLX[i], gFadeLY[i]);
		LogSilentF(" %.02f %.02f", gFadeRX[i], gFadeRY[i]);
#endif
	}

	// Phase 3: Calculate intermediate fade point
#ifdef FAN_STEP_DEBUG
	LogSilent("Intermediate points");
#endif // FAN_STEP_DEBUG
#define NEWER_INTER_FADE_CODE
//#define NEW_INTER_FADE_CODE
#ifdef NEWER_INTER_FADE_CODE
	pRay = pStart;
#endif
	bool *fAscend = new bool[iRayCnt];
	for (i = 0; i+1 < iRayCnt; i++) {

		// Calculate light bounds. We assume a "smaller" light for closer rays
		CALC_LIGHT(gFanRX[i], gFanRY[i], 1, gLightLX, gLightLY);
		CALC_LIGHT(gFanLX[i+1], gFanLY[i+1], -1, gLightRX, gLightRY);

#ifdef NEWER_INTER_FADE_CODE
		// Midpoint
		float mx = (gFadeRX[i] + gFadeLX[i+1]) / 2,
		      my = (gFadeRY[i] + gFadeLY[i+1]) / 2;
		while (pRay->getNext() && pRay->isRight(mx, my))
			pRay = pRay->getNext();
#endif

		// Ascending?
		fAscend[i] = gFanRY[i] > gFanLY[i+1];
		if (gFanRY[i] > gFanLY[i+1]) {

#ifdef NEWER_INTER_FADE_CODE

			float dx, dy;
			find_cross(0,0, mx, my,
			           pRay->getLeftEndXf(), pRay->getLeftEndY(), pRay->getRightEndXf(), pRay->getRightEndY(),
					   &dx, &dy);
			float d = float(pLight->getFadeout()) / sqrt(dx*dx + dy*dy);
			gFadeIX[i] = mx + d * dx;
			gFadeIY[i] = my + d * dy;
			if (gFadeRY[i] < gFadeIY[i]) {
				gFadeIX[i] = gFadeLX[i+1];
				gFadeIY[i] = gFadeLY[i+1];
			}

#elif defined(NEW_INTER_FADE_CODE)

			// Fade intermediate point is on the left side
			float dx = gFanRX[i] - gLightLX;
			float dy = gFanRY[i] - gLightLY;
			float d = float(pLight->getFadeout()) / sqrt(dx*dx + dy*dy);
			gFadeIX[i] = gFanRX[i] + d * dx;
			gFadeIY[i] = gFanRY[i] + d * dy;

#else
			// Fade intermediate point is on the right side
			gFadeIX[i] = gFadeLX[i+1];
			gFadeIY[i] = gFadeLY[i+1];
			
			// Project on left ray's height where necessary
			if (gFadeIY[i] < gFanRY[i]) {
				float d = (gFanRY[i] - gFadeIY[i]) / (gFadeIY[i] - gLightY);
				gFadeIX[i] += d * (gFadeIX[i] - gLightRX);
				gFadeIY[i] = gFanRY[i];
			}
#endif

		// Descending?
		} else {

#ifdef NEWER_INTER_FADE_CODE

			float dx, dy;
			find_cross(0,0, mx,my,
			           pRay->getLeftEndXf(), pRay->getLeftEndY(), pRay->getRightEndXf(), pRay->getRightEndY(),
					   &dx, &dy);
			float d = float(pLight->getFadeout()) / sqrt(dx*dx + dy*dy) / 2;
			gFadeIX[i] = mx + d * dx;
			gFadeIY[i] = my + d * dy;
			if (gFadeLY[i+1] < gFadeIY[i]) {
				gFadeIX[i] = gFadeRX[i];
				gFadeIY[i] = gFadeRY[i];
			}

#elif defined(NEW_INTER_FADE_CODE)
			
			// Fade intermediate point is on the right side
			float dx = gFanLX[i+1] - gLightRX;
			float dy = gFanLY[i+1] - gLightRY;
			float d = float(pLight->getFadeout()) / sqrt(dx*dx + dy*dy);
			gFadeIX[i] = gFanLX[i+1] + d * dx;
			gFadeIY[i] = gFanLY[i+1] + d * dy;

#else
			// Fade intermediate point is on the left side
			gFadeIX[i] = gFadeRX[i];
			gFadeIY[i] = gFadeRY[i];
			
			// Project on right ray's height where necessary
			if (gFadeIY[i] < gFanLY[i+1]) {
				float d = (gFanLY[i+1] - gFadeIY[i]) / (gFadeIY[i] - gLightY);
				gFadeIX[i] += d * (gFadeIX[i] - gLightLX);
				gFadeIY[i] = gFanLY[i+1];
			}
#endif

		}

#ifdef FAN_STEP_DEBUG
		LogSilentF(" %.02f %.02f", gFadeIX[i], gFadeIY[i]);
#endif // FAN_STEP_DEBUG
	}

	// Phase 4: Transform all points into region coordinates
	for (i = 0; i < 5; i++) {
		float *pX = gFanLX + 2 * i * iOriginalRayCnt,
			  *pY = gFanLY + 2 * i * iOriginalRayCnt;
		for (int32_t j = 0; j < iRayCnt; j++) {
			float x = pX[j], y = pY[j];
			if (pOnScreen)
			{
				pX[j] = float(pOnScreen->X) + transX(x, y) - pOnScreen->TargetX,
				pY[j] = float(pOnScreen->Y) + transY(x, y) - pOnScreen->TargetY;
				pGL->ApplyZoom(pX[j], pY[j]);
			}
			else
			{
				pX[j] = transX(x, y) - pRegion->getRegion().x;
				pY[j] = transY(x, y) - pRegion->getRegion().y;
			}
		}
	}

	// Calculate position of the light in the buffer
	float gLightX = transX(0,0) - pRegion->getRegion().x,
	      gLightY = transY(0,0) - pRegion->getRegion().y;

	// Here's the master plan for updating the lights texture. We
	// want to add intensity (R channel) as well as the normal (GB channels).
	// Normals are obviously meant to be though of as signed, though,
	// so the equation we want would be something like
	//
	//  R_new = BoundBy(R_old + R,       0.0, 1.0)
	//  G_new = BoundBy(G_old + G - 0.5, 0.0, 1.0)
	//  B_new = BoundBy(B_old + B - 0.5, 0.0, 1.0)
	//
	// It seems we can't get that directly though - glBlendFunc only talks
	// about two operands. Even if we make two passes, we have to take
	// care that that we don't over- or underflow in the intermediate pass.
	//
	// Therefore, we store G/1.5 instead of G, losing a bit of accuracy,
	// but allowing us to formulate the following approximation without
	// overflows:
	//
	//  G_new = BoundBy(BoundBy(G_old + G / 1.5), 0.0, 1.0) - 0.5 / 1.5, 0.0, 1.0)
	//  B_new = BoundBy(BoundBy(B_old + B / 1.5), 0.0, 1.0) - 0.5 / 1.5, 0.0, 1.0)

	// Two passes
	for(int iPass = 0; iPass < (pOnScreen ? 1 : 2); iPass++) {  

		// Pass 2: Subtract
		if (!pOnScreen && iPass == 1) {
			glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
			glBlendFunc(GL_ONE, GL_ONE);
		}

		// Help me! My brain can't program without local function definitions anymore!
		#define VERTEX(x,y,light)                                        \
			if(pOnScreen) {                                              \
				if(light)       glColor3f(1.0f, 0.0f, 0.0f);             \
				else            glColor3f(0.5f, 0.5f, 0.0f);             \
			} else if(iPass == 0) {                                      \
			    float dx = (x) - gLightX, dy = (y) - gLightY;            \
				float gDist = sqrt(dx*dx+dy*dy);                         \
				float gMult = Min(0.5f / pLight->getSize(), 0.5f / gDist); \
				float gNormX = (0.5f + dx * gMult) / 1.5f / 16.0f; \
				float gNormY = (0.5f + dy * gMult) / 1.5f / 16.0f; \
				if(light)       glColor3f(0.5f/16.0f, gNormX, gNormY);         \
				else            glColor3f(0.0f, gNormX, gNormY);         \
			} else              glColor3f(0.0f, 0.5f/1.5f/16.0f, 0.5f/1.5f/16.0f);   \
			glVertex2f(x,y)
		#define DARK(x,y) VERTEX(x,y,false)
		#define LIGHT(x,y) VERTEX(x,y,true)
		#define BEGIN_TRIANGLE                                           \
			if(pOnScreen) glBegin(GL_LINE_LOOP)
		#define END_TRIANGLE                                             \
			if(pOnScreen) glEnd()

		// Draw the fan
		glShadeModel(GL_SMOOTH);
		glBegin(pOnScreen ? GL_LINE_STRIP : GL_TRIANGLE_FAN);
		if (!pOnScreen) {
			LIGHT(gLightX, gLightY);
		}
		for (i = 0; i < iRayCnt; i++) {
			if (i == 0 || gFanRX[i-1] != gFanLX[i] || gFanRY[i-1] != gFanLY[i]) {
				LIGHT(gFanLX[i], gFanLY[i]);
			}
			if (gFanLX[i] != gFanRX[i] || gFanLY[i] != gFanRY[i]) {
				LIGHT(gFanRX[i], gFanRY[i]);
			}
		}
		glEnd();

		// Draw the fade
		glShadeModel(GL_SMOOTH);
		if(!pOnScreen) glBegin(GL_TRIANGLES);

		for (i = 0; i < iRayCnt; i++) {

			// The quad. Will be empty if fan points match
			if (gFanLX[i] != gFanRX[i] || gFanLY[i] != gFanRY[i]) {

				// upper triangle
				BEGIN_TRIANGLE;
				LIGHT(gFanLX[i], gFanLY[i]);
				LIGHT(gFanRX[i], gFanRY[i]);
				DARK(gFadeLX[i], gFadeLY[i]);
				END_TRIANGLE;

				// lower triangle, if necessary
				if (gFadeLX[i] != gFadeRX[i] || gFadeLY[i] != gFadeRY[i]) {
					BEGIN_TRIANGLE;
					LIGHT(gFanRX[i], gFanRY[i]);
					DARK(gFadeRX[i], gFadeRY[i]);
					DARK(gFadeLX[i], gFadeLY[i]);
					END_TRIANGLE;
				}
			}

			// No intermediate fade for last point
			if (i+1 >= iRayCnt) continue;

			// Ascending?
			if (fAscend[i]) {

				// Lower fade triangle
				BEGIN_TRIANGLE;
				LIGHT(gFanRX[i], gFanRY[i]);
				DARK(gFadeIX[i], gFadeIY[i]);
				DARK(gFadeRX[i], gFadeRY[i]);
				END_TRIANGLE;

				// Intermediate fade triangle, if necessary
				if (gFadeIY[i] != gFadeLY[i+1]) {
					BEGIN_TRIANGLE;
					LIGHT(gFanRX[i], gFanRY[i]);
					DARK(gFadeLX[i+1], gFadeLY[i+1]);
					DARK(gFadeIX[i], gFadeIY[i]);
					END_TRIANGLE;
				}

				// Upper fade triangle
				BEGIN_TRIANGLE;
				LIGHT(gFanRX[i], gFanRY[i]);
				LIGHT(gFanLX[i+1], gFanLY[i+1]);
				DARK(gFadeLX[i+1], gFadeLY[i+1]);
				END_TRIANGLE;

			// Descending?
			} else {

				// Lower fade triangle
				BEGIN_TRIANGLE;
				LIGHT(gFanLX[i+1], gFanLY[i+1]);
				DARK(gFadeLX[i+1], gFadeLY[i+1]);
				DARK(gFadeIX[i], gFadeIY[i]);
				END_TRIANGLE;

				// Intermediate fade triangle, if necessary
				if (gFadeIY[i] != gFadeRY[i]) {
					BEGIN_TRIANGLE;
					LIGHT(gFanLX[i+1], gFanLY[i+1]);
					DARK(gFadeIX[i], gFadeIY[i]);
					DARK(gFadeRX[i], gFadeRY[i]);
					END_TRIANGLE;
				}

				// Upper fade triangle
				BEGIN_TRIANGLE;
				LIGHT(gFanLX[i+1], gFanLY[i+1]);
				DARK(gFadeRX[i], gFadeRY[i]);
				LIGHT(gFanRX[i], gFanRY[i]);
				END_TRIANGLE;

			}
		}
		if (!pOnScreen)
			glEnd(); // GL_TRIANGLES
	}

	delete[] gFanLX;
	
	// Reset GL state
	glBlendEquation(GL_FUNC_ADD);

}


StdStrBuf C4FoWRay::getDesc() const {
	return FormatString("%d:%d@%d:%d%s",
		getLeftX(1000),
		getRightX(1000),
		getLeftEndY(),
		getRightEndY(),
		fDirty ? "*" : "");
}

bool C4FoWLightSection::isConsistent() const {
	return (a * c + b * d == 1) && (ra * rc + rb * rd == 1) &&
		   (a * ra + b * rc == 1) && (a * rb + b * rd == 0) &&
		   (c * ra + d * rc == 0) && (c * rb + d * rd == 1);
}
