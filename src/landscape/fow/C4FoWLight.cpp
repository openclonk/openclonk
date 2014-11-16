
#include "C4Include.h"
#include "C4FoWLight.h"
#include "C4FoWLightSection.h"
#include "C4FoWBeamTriangle.h"
#include "C4FoWDrawStrategy.h"

C4FoWLight::C4FoWLight(C4Object *pObj)
	: iX(fixtoi(pObj->fix_x)),
	  iY(fixtoi(pObj->fix_y)),
	  iReach(pObj->PlrViewRange),
	  iFadeout(50),
	  iSize(20),
	  pNext(NULL),
	  pObj(pObj),
	  sectionUp(this, 0),
	  sectionLeft(this, 270),
	  sectionDown(this, 180),
	  sectionRight(this, 90)
{
}

C4FoWLight::~C4FoWLight()
{
}

void C4FoWLight::Invalidate(C4Rect r)
{
	sectionUp.Invalidate(r);
	sectionDown.Invalidate(r);
	sectionLeft.Invalidate(r);
	sectionRight.Invalidate(r);
}

void C4FoWLight::SetReach(int32_t iReach2, int32_t iFadeout2)
{
	// Fadeout changes don't matter
	iFadeout = iFadeout2;

	if (iReach == iReach2) return;

	// Reach decreased? Prune beams
	if (iReach2 < iReach)
	{
		iReach = iReach2;
		sectionUp.Prune(iReach);
		sectionDown.Prune(iReach);
		sectionLeft.Prune(iReach);
		sectionRight.Prune(iReach);

	} else {

		// Reach increased? Dirty beams that might get longer now
		iReach = iReach2;
		sectionUp.Dirty(iReach);
		sectionDown.Dirty(iReach);
		sectionLeft.Dirty(iReach);
		sectionRight.Dirty(iReach);
	}
}

void C4FoWLight::Update(C4Rect Rec)
{

	// Update position from object. Clear if we moved in any way
	int32_t iNX = fixtoi(pObj->fix_x), iNY = fixtoi(pObj->fix_y);
	if (iNX != iX || iNY != iY)
	{
		sectionUp.Prune(0);
		sectionDown.Prune(0);
		sectionLeft.Prune(0);
		sectionRight.Prune(0);
		iX = iNX; iY = iNY;
	}

	sectionUp.Update(Rec);
	sectionDown.Update(Rec);
	sectionLeft.Update(Rec);
	sectionRight.Update(Rec);
}

void C4FoWLight::Render(C4FoWRegion *region, const C4TargetFacet *onScreen)
{
	std::list<C4FoWBeamTriangle> triangles;
	triangles.splice(triangles.end(), sectionUp.CalculateTriangles(region));
	triangles.splice(triangles.end(), sectionRight.CalculateTriangles(region));
	triangles.splice(triangles.end(), sectionDown.CalculateTriangles(region));
	triangles.splice(triangles.end(), sectionLeft.CalculateTriangles(region));

	CalculateIntermediateFadeTriangles(triangles);
	
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

	C4FoWDrawStrategy* pen;
	if (onScreen) pen = new C4FoWDrawWireframeStrategy(this, onScreen);
	else          pen = new C4FoWDrawLightTextureStrategy(this, region);

	for(int pass = 0; pass < pen->GetRequestedPasses(); pass++)
	{  
		pen->Begin(pass);

		DrawFan(pen, triangles);
		DrawFade(pen, triangles);
		DrawIntermediateFadeTriangles(pen, triangles);

		pen->End(pass);
	}

	delete pen;
}

void C4FoWLight::CalculateIntermediateFadeTriangles(std::list<class C4FoWBeamTriangle> &triangles)
{
	for (std::list<C4FoWBeamTriangle>::iterator it = triangles.begin(), nextIt = it; it != triangles.end(); ++it)
	{
		// wrap around
		++nextIt;
		if(nextIt == triangles.end()) nextIt = triangles.begin();

		C4FoWBeamTriangle &tri = *it, &nextTri = *nextIt; // just for convenience

		// don't calculate if it should not be drawn anyway
		if (tri.clipRight || nextTri.clipLeft) continue;

		// Midpoint
		tri.fadeIX = (tri.fadeRX + nextTri.fadeLX) / 2;
		tri.fadeIY = (tri.fadeRY + nextTri.fadeLY) / 2;

		float distFanR = GetSquaredDistanceTo(tri.fanRX, tri.fanRY);
		float distNextFanL = GetSquaredDistanceTo(nextTri.fanLX, nextTri.fanLY);
		float distFadeI = GetSquaredDistanceTo(tri.fadeIX, tri.fadeIY);

		// an intermediate fade point is not necessery in all cases
		tri.descending = distFanR > distNextFanL;
		if (tri.descending) {
			float distFadeR = GetSquaredDistanceTo(tri.fadeRX, tri.fadeRY);
			if (distFadeR < distFadeI)
			{
				tri.fadeIX = nextTri.fadeLX;
				tri.fadeIY = nextTri.fadeLY;
			}
		}
		else
		{
			float distNextFadeL = GetSquaredDistanceTo(nextTri.fadeLX, nextTri.fadeLY);
			if (nextTri.fadeLY < tri.fadeIY)
			{
				tri.fadeIX = tri.fadeRX;
				tri.fadeIY = tri.fadeRY;
			}
		}
	}
}

void C4FoWLight::DrawFan(C4FoWDrawStrategy* pen, std::list<C4FoWBeamTriangle> &triangles)
{
	pen->BeginFan();
	pen->DrawLightVertex(getX(), getY());

	for (std::list<C4FoWBeamTriangle>::iterator it = triangles.begin(), nextIt = it; it != triangles.end(); ++it)
	{
		// wrap around
		++nextIt;
		if(nextIt == triangles.end()) nextIt = triangles.begin();

		C4FoWBeamTriangle &tri = *it, &nextTri = *nextIt; // just for convenience

		pen->DrawLightVertex(tri.fanLX, tri.fanLY);	
		pen->DrawLightVertex(tri.fanRX, tri.fanRY);
	}
	pen->EndFan();
}

void C4FoWLight::DrawFade(C4FoWDrawStrategy* pen, std::list<C4FoWBeamTriangle> &triangles)
{
	pen->BeginFade();

	for (std::list<C4FoWBeamTriangle>::iterator it = triangles.begin(); it != triangles.end(); ++it)
	{
		C4FoWBeamTriangle &tri = *it; // just for convenience

		// The quad will be empty if fan points match
		if (tri.fanLX == tri.fanRX && tri.fanLY == tri.fanRY) continue;

		pen->DrawLightVertex(tri.fanLX, tri.fanLY);
		pen->DrawLightVertex(tri.fanRX, tri.fanRY);
		pen->DrawDarkVertex(tri.fadeRX, tri.fadeRY);
		pen->DrawDarkVertex(tri.fadeLX, tri.fadeLY);
	}
	pen->EndFade();
}

void C4FoWLight::DrawIntermediateFadeTriangles(C4FoWDrawStrategy* pen, std::list<C4FoWBeamTriangle> &triangles)
{
	pen->BeginIntermediateFade();

	for (std::list<C4FoWBeamTriangle>::iterator it = triangles.begin(), nextIt = it; it != triangles.end(); ++it)
	{
		// wrap around
		++nextIt;
		if(nextIt == triangles.end()) nextIt = triangles.begin();

		C4FoWBeamTriangle &tri = *it, &nextTri = *nextIt; // just for convenience
			
		// no inter-fade triangles when it should be clipped
		if (tri.clipRight || nextTri.clipLeft) continue;

		if (tri.descending) {

			// Lower fade triangle
			pen->DrawLightVertex(tri.fanRX, tri.fanRY);
			pen->DrawDarkVertex(tri.fadeIX, tri.fadeIY);
			pen->DrawDarkVertex(tri.fadeRX, tri.fadeRY);

			// Intermediate fade triangle, if necessary
			if (tri.fadeIY != nextTri.fadeRY || tri.fadeIX != nextTri.fadeRX) {
				pen->DrawLightVertex(tri.fanRX, tri.fanRY);
				pen->DrawDarkVertex(nextTri.fadeLX, nextTri.fadeLY);
				pen->DrawDarkVertex(tri.fadeIX, tri.fadeIY);
			}

			// Upper fade triangle
			pen->DrawLightVertex(tri.fanRX, tri.fanRY);
			pen->DrawLightVertex(nextTri.fanLX, nextTri.fanLY);
			pen->DrawDarkVertex(nextTri.fadeLX, nextTri.fadeLY);

		} else {

			// Lower fade triangle
			pen->DrawLightVertex(nextTri.fanLX, nextTri.fanLY);
			pen->DrawDarkVertex(nextTri.fadeLX, nextTri.fadeLY);
			pen->DrawDarkVertex(tri.fadeIX, tri.fadeIY);

			// Intermediate fade triangle, if necessary
			if (tri.fadeIY != nextTri.fadeRY || tri.fadeIX != nextTri.fadeRX) {
				pen->DrawLightVertex(nextTri.fanLX, nextTri.fanLY);
				pen->DrawDarkVertex(tri.fadeIX, tri.fadeIY);
				pen->DrawDarkVertex(tri.fadeRX, tri.fadeRY);
			}

			// Upper fade triangle
			pen->DrawLightVertex(nextTri.fanLX, nextTri.fanLY);
			pen->DrawDarkVertex(tri.fadeRX, tri.fadeRY);
			pen->DrawLightVertex(tri.fanRX, tri.fanRY);
		}
	}
	pen->EndIntermediateFade();
}
