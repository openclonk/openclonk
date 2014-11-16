
#include "C4Include.h"
#include "C4FoWLightSection.h"
#include "C4FoWBeamTriangle.h"
#include "C4FoWBeam.h"
#include "C4FoWLight.h"
#include "C4FoWRegion.h"
#include "C4Landscape.h"

#include "float.h"

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

C4FoWLightSection::C4FoWLightSection(C4FoWLight *pLight, int r) : pLight(pLight), iRot(r)
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
	// Beam list
	pBeams = new C4FoWBeam(-1, +1, +1, +1);
}

C4FoWLightSection::~C4FoWLightSection()
{
	ClearBeams();
}

inline void C4FoWLightSection::LightBallExtremePoint(float x, float y, float dir, float &lightX, float &lightY) const
{
	float d = sqrt(x * x + y * y);
	float s = Min(float(pLight->getSize()), d / 5.0f);
	lightX = dir * y * s / d;
	lightY = dir * -x * s / d;
}

inline void C4FoWLightSection::LightBallRightMostPoint(float x, float y, float &lightX, float &lightY) const
	{ LightBallExtremePoint(x,y,+1.0f,lightX,lightY); }

inline void C4FoWLightSection::LightBallLeftMostPoint(float x, float y, float &lightX, float &lightY) const
	{ LightBallExtremePoint(x,y,-1.0f,lightX,lightY); }

template <class T> T C4FoWLightSection::transDX(T dx, T dy) const { return T(a) * dx + T(b) * dy; }
template <class T> T C4FoWLightSection::transDY(T dx, T dy) const { return T(c) * dx + T(d) * dy; }
template <class T> T C4FoWLightSection::transX(T x, T y) const { return transDX(x, y) + T(pLight->getX()); }
template <class T> T C4FoWLightSection::transY(T x, T y) const { return transDY(x, y) + T(pLight->getY()); }

template <class T> T C4FoWLightSection::rtransDX(T dx, T dy) const { return T(ra) * dx + T(rb) * dy; }
template <class T> T C4FoWLightSection::rtransDY(T dx, T dy) const { return T(rc) * dx + T(rd) * dy; }
template <class T> T C4FoWLightSection::rtransX(T x, T y) const { return rtransDX(x-T(pLight->getX()),y-T(pLight->getY())); }
template <class T> T C4FoWLightSection::rtransY(T x, T y) const { return rtransDY(x-T(pLight->getX()),y-T(pLight->getY())); }

void C4FoWLightSection::ClearBeams()
{
	while (C4FoWBeam *pBeam = pBeams) {
		pBeams = pBeam->getNext();
		delete pBeam;
	}
}

void C4FoWLightSection::Prune(int32_t iReach)
{
	if (iReach == 0) {
		ClearBeams();
		pBeams = new C4FoWBeam(-1, 1, 1, 1);
		return;
	}
	// TODO PeterW: Merge active beams that we have pruned to same length
	for (C4FoWBeam *pBeam = pBeams; pBeam; pBeam = pBeam->getNext())
		pBeam->Prune(iReach);
}

void C4FoWLightSection::Dirty(int32_t iReach)
{
	for (C4FoWBeam *pBeam = pBeams; pBeam; pBeam = pBeam->getNext())
		if (pBeam->getLeftEndY() >= iReach || pBeam->getRightEndY() >= iReach)
			pBeam->Dirty(Min(pBeam->getLeftEndY(), pBeam->getRightEndY()));
}

C4FoWBeam *C4FoWLightSection::FindBeamLeftOf(int32_t x, int32_t y)
{
	// Trivial
	y = Max(y, 0);
	if (!pBeams || !pBeams->isRight(x, y))
		return NULL;
	// Go through list
	// Note: In case this turns out expensive, one might think about implementing
	// a skip-list. But I highly doubt it.
	C4FoWBeam *pBeam = pBeams;
	while (pBeam->getNext() && pBeam->getNext()->isRight(x, y))
		pBeam = pBeam->getNext();
	return pBeam;
}

void C4FoWLightSection::Update(C4Rect RectIn)
{

	// Transform rectangle into our coordinate system
	C4Rect Rect = rtransRect(RectIn);
	C4Rect Bounds = rtransRect(C4Rect(0,0,GBackWdt,GBackHgt));

#ifdef LIGHT_DEBUG
	if (!::Game.iTick255) {
		LogSilentF("Full beam list:");
		StdStrBuf Beams;
		for(C4FoWBeam *pBeam = pBeams; pBeam; pBeam = pBeam->getNext()) {
			Beams.AppendChar(' ');
			Beams.Append(pBeam->getDesc());
		}
		LogSilent(Beams.getData());
	}
#endif

#ifdef LIGHT_DEBUG
	LogSilentF("Update %d/%d-%d/%d", Rect.x, Rect.y, Rect.x+Rect.Wdt, Rect.y+Rect.Hgt);
#endif

	// Out of reach?
	if (Rect.y > pLight->getTotalReach())
		return;

	// will be clipped on rendering anyway, don't bother to run the algorithm
	if (Rect.y + Rect.Hgt < 0)
		return;
	
	// Get last beam that's positively *not* affected
	int32_t iLY = RectLeftMostY(Rect),
	        iLX = RectLeftMostX(Rect),
	        iRY = RectRightMostY(Rect),
	        iRX = RectRightMostX(Rect);

	C4FoWBeam *pStart = FindBeamLeftOf(iLX, iLY);

	// Skip clean beams
	while (C4FoWBeam *pNext = pStart ? pStart->getNext() : pBeams) {
		if (pNext->isDirty()) break;
		pStart = pNext;
	}
	// Find end beam, determine at which position we have to start scanning
	C4FoWBeam *pBeam = pStart ? pStart->getNext() : pBeams;
#ifdef LIGHT_DEBUG
	if (pBeam)
		LogSilentF("Start beam is %s", pBeam->getDesc().getData());
#endif
	C4FoWBeam *pEnd = NULL;
	int32_t iStartY = Rect.GetBottom();
	while (pBeam && !pBeam->isLeft(iRX, iRY)) {
		if (pBeam->isDirty() && pBeam->getLeftEndY() <= Rect.y+Rect.Hgt) {
			pEnd = pBeam;
			iStartY = Min(iStartY, pBeam->getLeftEndY());
		}
		pBeam = pBeam->getNext();
	}

	// Can skip scan completely?
	if (!pEnd)
		return;

	// Update right end coordinates
#ifdef LIGHT_DEBUG
	LogSilentF("End beam is %s", pEnd->getDesc().getData());
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

		// Scan all beams
		C4FoWBeam *pLast = pStart; int32_t iDirty = 0;
		for(C4FoWBeam *pBeam = pStart ? pStart->getNext() : pBeams; pBeam; pLast = pBeam, pBeam = pBeam->getNext()) {
			assert(pLast ? pLast->getNext() == pBeam : pBeam == pBeams);

			// Clean (enough)?
			if (!pBeam->isDirty() || y < pBeam->getLeftEndY())
				continue;

			// Out left?
			if (pBeam->isRight(iLX, y))
				continue;
			// Out right?
			if (pBeam->isLeft(iRX, y) || pBeam->isLeft(iRX, iRY))
				break;

			// We have an active beam that we're about to scan
			iDirty++;
			pBeam->Dirty(y+1);

			// Do a scan
			int32_t xl = Max(pBeam->getLeftX(y), Bounds.x),
			        xr = Min(pBeam->getRightX(y), Bounds.x+Bounds.Wdt-1);
			for(int32_t x = xl; x <= xr; x++) {

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
				int32_t x1 = x - 1, x2 = x + 1;
				bool fSplitLeft = !pBeam->isLeft(x1, y);
				bool fSplitRight = !pBeam->isRight(x2, y);

				// Double merge?
				if (!fSplitLeft && !fSplitRight && pLast && pBeam->getNext()) {
					if(pLast->EliminateRight(x,y)) {
						pBeam = pLast;
						break; // no typo. fSplitRight => x == xr
					}
				}

				// Merge possible?
				if (!fSplitLeft && fSplitRight && pLast)
					if (pLast->MergeRight(x2, y)) {
						pBeam->SetLeft(x2, y);
						assert(pBeam->isDirty());
						continue;
					}
				if (fSplitLeft && !fSplitRight && pBeam->getNext())
					if (pBeam->getNext()->MergeLeft(x1, y)) {
						pBeam->SetRight(x1, y);
						break; // no typo. fSplitRight => x == xr
					}

				// Split out left
				if (fSplitLeft) {
					pLast = pBeam;
					pBeam = pLast->Split(x1, y);
					assert(pLast->getNext() == pBeam);
				}

				// Split out right
				if(fSplitRight) {
					pLast = pBeam;
					pBeam = pLast->Split(x2, y);
					assert(pLast->getNext() == pBeam);

					// Deactivate left/middle beam
					pLast->Clean(y);
					assert(pBeam->isDirty());

				} else {

					// Deactivate beam
					pBeam->Clean(y);
					break;

				}

			}

		}

		// No active beams left?
		if (!iDirty)
			break;
			
	}

	// At end of light's reach? Mark all beams that got scanned all the way to the end as clean.
	// There's no need to scan them anymore.
	if (y >= pLight->getReach()) {
		for (C4FoWBeam *pBeam = pStart ? pStart->getNext() : pBeams; pBeam; pBeam = pBeam->getNext())
			if (pBeam->isDirty() && pBeam->getLeftEndY() > pLight->getReach())
				pBeam->Clean(pLight->getReach());
	}

#ifdef LIGHT_DEBUG
	LogSilentF("Updated beam list:");
	for(C4FoWBeam *pBeam = pStart ? pStart->getNext() : pBeams; pBeam; pBeam = pBeam->getNext()) {
		if (pBeam->isLeft(iRX, iRY))
			break;
		LogSilent(pBeam->getDesc().getData());
	}
#endif
}



bool C4FoWLightSection::isConsistent() const {
	return (a * c + b * d == 1) && (ra * rc + rb * rd == 1) &&
		   (a * ra + b * rc == 1) && (a * rb + b * rd == 0) &&
		   (c * ra + d * rc == 0) && (c * rb + d * rd == 1);
}

void C4FoWLightSection::Invalidate(C4Rect r)
{
	// Assume normalized rectangle
	assert(r.Wdt > 0 && r.Hgt > 0);
	
	// Get rectangle corners that bound the possibly affected beams
	int32_t iLY = RectLeftMostY(r),
	        iLX = RectLeftMostX(r),
	        iRY = RectRightMostY(r),
	        iRX = RectRightMostX(r);
	C4FoWBeam *pLast = FindBeamLeftOf(iLX, iLY);
	C4FoWBeam *pBeam = pLast ? pLast->getNext() : pBeams;

	// Scan over beams
	while (pBeam && !pBeam->isLeft(iRX, iRY)) {

		// Dirty beam?
		if (pBeam->getLeftEndY() > r.y || pBeam->getRightEndY() > r.y)
			pBeam->Dirty(r.y);

		// Merge with last beam?
		if (pLast && pLast->isDirty() && pBeam->isDirty()) {
			pLast->MergeDirty();
			pBeam = pLast->getNext();

		// Advance otherwise
		} else {
			pLast = pBeam;
			pBeam = pBeam->getNext();
		}
	}

	// Final check for merging dirty beams on the right end
	if (pLast && pBeam && pLast->isDirty() && pBeam->isDirty())
		pLast->MergeDirty();

}

int32_t C4FoWLightSection::FindBeamsClipped(const C4Rect &pInRect, C4FoWBeam *&pFirst, C4FoWBeam *&pLast)
{
	if(pInRect.y + pInRect.Hgt < 0) return 0;

	int32_t iLY = RectLeftMostY(pInRect),
	        iLX = RectLeftMostX(pInRect),
	        iRY = RectRightMostY(pInRect),
	        iRX = RectRightMostX(pInRect);

	C4FoWBeam *pPrev = FindBeamLeftOf(iLX, iLY);
	pFirst = pPrev ? pPrev->getNext() : pBeams;

	// Find end beam - determine the number of beams we actually need to draw
	C4FoWBeam *pBeam = pFirst;
	int32_t iBeamCount = 0;
	while (pBeam && !pBeam->isLeft(iRX, iRY))
	{
		pBeam = pBeam->getNext();
		iBeamCount++;
	}
	pLast = pBeam;

	return iBeamCount;
}

std::list<C4FoWBeamTriangle> C4FoWLightSection::CalculateTriangles(C4FoWRegion *pRegion)
{
	C4FoWBeam *pStart = NULL, *pEnd = NULL;
	int32_t iBeamCount = FindBeamsClipped(rtransRect(pRegion->getRegion()), pStart, pEnd);
	// no beams inside the rectangle? Good, nothing to render 
	std::list<C4FoWBeamTriangle> result;
	if(!iBeamCount) return result;

	int32_t iOriginalBeamCount = iBeamCount;

	// Allocate array for our points (lots of them)
	float *gFanLX = new float [iBeamCount * 10],	// positions where the left lines of beams end
		  *gFanLY = gFanLX + iBeamCount,			
		  *gFanRX = gFanLY + iBeamCount,			// positions where the right lines of beams end
		  *gFanRY = gFanRX + iBeamCount,
		  *gFadeLX = gFanRY + iBeamCount,
		  *gFadeLY = gFadeLX + iBeamCount,
		  *gFadeRX = gFadeLY + iBeamCount,
		  *gFadeRY = gFadeRX + iBeamCount;
	int32_t i;
	C4FoWBeam *pBeam = pStart;
	for (i = 0, pBeam = pStart; i < iBeamCount; i++, pBeam = pBeam->getNext()) {
		gFanLX[i] = pBeam->getLeftEndXf();
		gFanLY[i] = float(pBeam->getLeftEndY());
		gFanRX[i] = pBeam->getRightEndXf();
		gFanRY[i] = float(pBeam->getRightEndY());
	}

	// Phase 1: Project lower point so it lies on a line with outer left/right
	// light lines.
	float gScanLevel = 0;
	for (int iStep = 0; iStep < 100000; iStep++) {

		// Find the beam to project. This makes this whole algorithm O(n²),
		// but I see no other way to make the whole thing robust :/
		float gBestLevel = FLT_MAX;
		int j;
		for (j = 0; j+1 < iBeamCount; j++) {
			float gLevel = Min(gFanRY[j], gFanLY[j+1]);
			if (gLevel <= gScanLevel || gLevel >= gBestLevel)
				continue;
			gBestLevel = gLevel;
		}
		if(gBestLevel == FLT_MAX)
			break;
		gScanLevel = gBestLevel;

		for(int i = 0; i+1 < iBeamCount; i++) {

			if(Min(gFanRY[i], gFanLY[i+1]) != gBestLevel)
				continue;

			// Calculate light bounds. We assume a "smaller" light for closer beams
			float gLightLX, gLightLY, gLightRX, gLightRY;
			LightBallLeftMostPoint(gFanRX[i], gFanRY[i], gLightLX, gLightLY);
			LightBallRightMostPoint(gFanLX[i+1], gFanLY[i+1], gLightRX, gLightRY);

			// Ascending
			float gCrossX, gCrossY;
			bool fDescendCollision = false;
			if (gFanRY[i] > gFanLY[i+1]) {

				// Left beam surface self-shadowing? We test whether the scalar product
				// of the beam's normal and the light vector is positive.
				if (  (gFanRY[i] - gFanLY[i]) * (gFanLX[i+1] - gLightRX) >=
					  (gFanRX[i] - gFanLX[i]) * (gFanLY[i+1] - gLightRY)) {

					// Reduce to upper point (Yep, we know that the upper point
					// must be the right one. Try to figure out why!)
					assert(gFanRY[i] <= gFanLY[i]);
					gFanLX[i] = gFanRX[i];
					gFanLY[i] = gFanRY[i];
				}

				// Left beam reduced?
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

				// Move right point of left beam to the left (the original point is partly shadowed)
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
				// down far enough between this and the next beam to hit anything.
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
					for(int j = i; j+1 < iBeamCount; j++) {
						gFanLX[j] = gFanLX[j+1];
						gFanLY[j] = gFanLY[j+1];
						gFanRX[j] = gFanRX[j+1];
						gFanRY[j] = gFanRY[j+1];
					}

					// With the elimination, we need to re-process the last
					// beam, as it might be more shadowed than we realized.
					// Note that the last point might have been projected already - 
					// but that's okay, 
					iBeamCount--; i-=2;
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
				if (fEliminate && i+2 < iBeamCount) {
					for(int j = i+1; j+1 < iBeamCount; j++) {
						gFanLX[j] = gFanLX[j+1];
						gFanLY[j] = gFanLY[j+1];
						gFanRX[j] = gFanRX[j+1];
						gFanRY[j] = gFanRY[j+1];
					}
					iBeamCount--; i--;
				}
			}

			if (fDescendCollision) {

				// Should never be parallel -- otherwise we wouldn't be here
				// in the first place.
				bool f = find_cross(gLightLX, gLightLY, gFanRX[i], gFanRY[i],
									gLightRX, gLightRY, gFanLX[i+1], gFanLY[i+1],
									&gCrossX, &gCrossY);
				assert(f);

				// Ensure some minimum distance to existing
				// points - don't bother with too small
				// bumps. This also catches some floating
				// point inacurracies.
				const float gDescendEta = 0.5;
				if (gCrossY <= gFanRY[i] + gDescendEta ||
					gCrossY <= gFanLY[i+1] + gDescendEta)
				  continue;

				// This should always follow an elimination, but better check
				assert(iOriginalBeamCount > iBeamCount);
				for (int j = iBeamCount - 1; j >= i+1; j--) {
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

				// Jump over surface. Note that our right beam might get
				// eliminated later on, causing us to back-track into this
				// zero-length pseudo-surface. This will cause find_cross
				// above to eliminate the pseudo-surface and back-track
				// further to the left, which is exactly how it should work.
				iBeamCount++; i++;
			}

		} // end for(int i = 0; i+1 < iBeamCount; i++) loop
	} // end for (int iStep = 0; iStep < 100000; iStep++) loop

	// Phase 2: Calculate fade points
	for (i = 0; i < iBeamCount; i++) {

		// Calculate light bounds. Note that the way light size is calculated
		// and we are using it below, we need to consider an "asymetrical" light.
		float gLightLX, gLightLY, gLightRX, gLightRY;
		LightBallLeftMostPoint(gFanLX[i], gFanLY[i], gLightLX, gLightLY);
		LightBallRightMostPoint(gFanRX[i], gFanRY[i], gLightRX, gLightRY);
		
		// This is simply the projection of the left point using the left-most
		// light point, as well as the projection of the right point using the
		// right-most light point.

		// For once we actually calculate this using the real distance
		float dx = gFanLX[i] - gLightLX;
		float dy = gFanLY[i] - gLightLY;
		float d = float(pLight->getFadeout()) / sqrt(dx*dx + dy*dy);
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

	}

	// Phase 4: Transform all points into global coordinates
	for (i = 0; i < 4; i++) {
		float *pX = gFanLX + 2 * i * iOriginalBeamCount,
			  *pY = gFanLY + 2 * i * iOriginalBeamCount;
		for (int32_t j = 0; j < iBeamCount; j++) {
			float x = pX[j], y = pY[j];
			pX[j] = transX(x, y);
			pY[j] = transY(x, y);
		}
	}

	for (i = 0; i < iBeamCount; i++)
	{
		C4FoWBeamTriangle triangle = C4FoWBeamTriangle();
		triangle.fanLX = gFanLX[i];
		triangle.fanLY = gFanLY[i];
		triangle.fanRX = gFanRX[i];
		triangle.fanRY = gFanRY[i];
		triangle.fadeLX = gFadeLX[i];
		triangle.fadeLY = gFadeLY[i];
		triangle.fadeRX = gFadeRX[i];
		triangle.fadeRY = gFadeRY[i];
		triangle.clipLeft = false; // TODO Newton: pBeams.start != pStart
		triangle.clipRight = false; // TODO Newton: pBeams.end != pEnd
		result.push_back(triangle);
	}

	delete[] gFanLX;

	return result;
}

