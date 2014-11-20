
#include "C4Include.h"
#include "C4FoWLight.h"
#include "C4FoWLightSection.h"
#include "C4FoWBeamTriangle.h"
#include "C4FoWDrawStrategy.h"

#include <vector>

C4FoWLight::C4FoWLight(C4Object *pObj)
	: iX(fixtoi(pObj->fix_x)),
	  iY(fixtoi(pObj->fix_y)),
	  iReach(pObj->PlrViewRange),
	  iFadeout(50),
	  iSize(20),
	  pNext(NULL),
	  pObj(pObj),
	  sections(4)
{
	sections[0] = new C4FoWLightSection(this,0);
	sections[1] = new C4FoWLightSection(this,90);
	sections[2] = new C4FoWLightSection(this,180);
	sections[3] = new C4FoWLightSection(this,270);
}

C4FoWLight::~C4FoWLight()
{
	for( int i = 0; i < sections.size(); ++i )
		delete sections[i];
}

void C4FoWLight::Invalidate(C4Rect r)
{
	for( int i = 0; i < sections.size(); ++i )
		sections[i]->Invalidate(r);
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
		for( int i = 0; i < sections.size(); ++i )
			sections[i]->Prune(iReach);

	} else {

		// Reach increased? Dirty beams that might get longer now
		iReach = iReach2;
		for( int i = 0; i < sections.size(); ++i )
			sections[i]->Dirty(iReach);
	}
}

void C4FoWLight::Update(C4Rect Rec)
{
	// Update position from object. Clear if we moved in any way
	int32_t iNX = fixtoi(pObj->fix_x), iNY = fixtoi(pObj->fix_y);
	if (iNX != iX || iNY != iY)
	{
		for( int i = 0; i < sections.size(); ++i )
			sections[i]->Prune(0);
		iX = iNX; iY = iNY;
	}

	for( int i = 0; i < sections.size(); ++i )
		sections[i]->Update(Rec);
}

void C4FoWLight::Render(C4FoWRegion *region, const C4TargetFacet *onScreen)
{
	std::list<C4FoWBeamTriangle> triangles;

	bool clip = false;
	
	for( int i = 0; i < sections.size(); ++i )
	{
		std::list<C4FoWBeamTriangle> sectionTriangles = sections[i]->CalculateTriangles(region);

		// if the triangles of one section are clipped completely, the neighbouring triangles
		// must be marked as clipped
		if(!triangles.empty()) triangles.rbegin()->clipRight |= clip;
		if(!sectionTriangles.empty()) sectionTriangles.begin()->clipLeft |= clip;

		clip = sectionTriangles.empty();
		triangles.splice(triangles.end(), sectionTriangles);
	}
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
		
		float distFanR = GetSquaredDistanceTo(tri.fanRX, tri.fanRY);
		float distNextFanL = GetSquaredDistanceTo(nextTri.fanLX, nextTri.fanLY);

		// Midpoint
		float mx = (tri.fadeRX + nextTri.fadeLX) / 2;
		float my = (tri.fadeRY + nextTri.fadeLY) / 2;

		// we could set tri.fadeIX and Y to mx and my, but the following section makes certain
		// fades a bit smoother by extending the fade mid point further away from the light.
		// This is especially visible for sharp edges like just looking over a cliff (= the 
		// length of fanR and nextFanL is very different)
		float dx, dy;
		C4FoWBeamTriangle &largerTri = distFanR > distNextFanL ? tri : nextTri;
		find_cross(getX(), getY(), mx, my,
		           largerTri.fanLX, largerTri.fanLY, largerTri.fanRX, largerTri.fanRY,
		           &dx, &dy);
		// make dx, dy relative to light source
		dx -= getX();
		dy -= getY();
		float distanceDifference = float(getTotalReach()) / sqrt(dx*dx + dy*dy);
		tri.fadeIX = getX() + distanceDifference * dx;
		tri.fadeIY = getY() + distanceDifference * dy;
		// end section

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
			if (distNextFadeL < distFadeI)
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

		if(nextIt == triangles.begin() || nextTri.fanLX != tri.fanRX || nextTri.fanLY != tri.fanRY)
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

bool find_cross(float ax, float ay, float bx, float by,
                float px, float py, float qx, float qy,
				float *ix, float *iy, float *abParameter)
{

	float numerator =   (py - ay) * (bx - ax) - (by - ay) * (px - ax);
	float denominator = (qx - px) * (by - ay) - (qy - py) * (bx - ax);

	//  if the denominator is zero, the lines are parallel. If the numerator
	//  is zero, too, the lines are on the same line.
	if (denominator == 0)
	{
		if(numerator == 0)
		{
			// just return any point then
			*ix = ax;
			*iy = ay;
			if(abParameter) *abParameter = 0;
			return true;
		}
		else
		{
			return false;
		}
	}

	float pqParam = numerator / denominator;

	// One of them might be division by zero. We can use either equation to get the result.
	// Both denominators only could be /0 if this line is a point

	if(by - ay != 0)
	{
		numerator = py - ay + pqParam * (qy - py);
		denominator = by - ay;
	}
	else if(bx - ax != 0)
	{
		numerator = px - ax + pqParam * (qx - px);
		denominator = bx - ax;
	}
	else
	{
		return false;
	}

	float abParam = numerator / denominator;

	*ix = ax + abParam * (bx - ax);
	*iy = ay + abParam * (by - ay);

	if(abParameter) *abParameter = abParam;

	return true;
}