#ifndef C4FOWREGION_H
#define C4FOWREGION_H

#include "C4Rect.h"
#include "C4Surface.h"
#include "C4FacetEx.h"
#include "C4Player.h"
#include "C4FoW.h"

class C4FoWRegion
{
public:
	C4FoWRegion(C4FoW *pFoW, C4Player *pPlayer);
	~C4FoWRegion();

private:
	C4FoW *pFoW;
	C4Player *pPlayer;
	C4Rect Region, OldRegion;
	C4Surface *pSurface, *pBackSurface;
	GLuint hFrameBufDraw, hFrameBufRead;

public:
	const C4Rect &getRegion() const { return Region; }
	const C4Surface *getSurface() const { return pSurface; }
	const C4Surface *getBackSurface() const { return pBackSurface; }

	void Clear();

	void Update(C4Rect r);
	void Render(const C4TargetFacet *pOnScreen = NULL);

private:
	bool BindFramebuf();

};

#endif