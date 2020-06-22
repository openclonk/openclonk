/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

/* Finds the way through the Clonk landscape */

/* Notes

   09-30-99
   I have had the concept for this code for more than two years now.
   Finally, it is written.

   10-26-99
   C4PathFinderRay::Crawl use IsCrawlAttach instead of GetCrawlAttach (which
   might not correctly indicate attach loss). Otherwise 1 pixel clefts can lead
   to backward crawl looping. Surprised, I haven't noticed this before.
   Also do not check attach loss on first crawl for that be might diagonal.
   C4PF_Ray_Crawl try new ray: don't worry about checking backwards jump if
   path to target is all free.
   C4PathFinderRay::FindCrawlAttachDiagonal check according to desired
   direction or else might lead off to no attach.

   11-24-99
   TransferZones

   12-11-99
   SetCompletePath don't set move-to waypoint if setting use-zone waypoint (is
   done by C4Command::Transfer on demand and would only cause no-good-entry-point
   move-to's on crawl-zone-entries).

*/

#include "C4Include.h"
#include "landscape/C4PathFinder.h"

#include "game/C4GraphicsSystem.h"
#include "graphics/C4Draw.h"
#include "graphics/C4FacetEx.h"
#include "lib/StdColors.h"

const int32_t C4PF_MaxDepth        = 35,
              C4PF_MaxCrawl        = 800,
              C4PF_MaxRay          = 350,
              C4PF_Threshold       = 10,
              C4PF_Direction_Left  = -1,
              C4PF_Direction_Right = +1,
//              C4PF_Direction_None  = 0,
              C4PF_Ray_Launch      = 0,
              C4PF_Ray_Crawl       = 1,
              C4PF_Ray_Still       = 2,
              C4PF_Ray_Failure     = 3,
              C4PF_Ray_Deleted     = 4,
              C4PF_Crawl_NoAttach  = 0,
              C4PF_Crawl_Top       = 1,
              C4PF_Crawl_Right     = 2,
              C4PF_Crawl_Bottom    = 3,
              C4PF_Crawl_Left      = 4,
              C4PF_Draw_Rate       = 10;

//------------------------------- C4PathFinderRay ---------------------------------------------
class C4PathFinderRay
{
	friend class C4PathFinder;
public:
	C4PathFinderRay();
	~C4PathFinderRay();
public:
	void Clear();
	void Default();
protected:
	int32_t Status;
	int32_t X, Y, X2, Y2, TargetX, TargetY;
	int32_t CrawlStartX, CrawlStartY, CrawlAttach, CrawlLength, CrawlStartAttach;
	int32_t Direction, Depth;
	C4TransferZone *UseZone;
	C4PathFinderRay *From;
	C4PathFinderRay *Next;
	C4PathFinder *pPathFinder;
protected:
	void SetCompletePath();
	void TurnAttach(int32_t &rAttach, int32_t iDirection);
	void CrawlToAttach(int32_t &rX, int32_t &rY, int32_t iAttach);
	void CrawlByAttach(int32_t &rX, int32_t &rY, int32_t iAttach, int32_t iDirection);
	void Draw(C4TargetFacet &cgo);
	int32_t FindCrawlAttachDiagonal(int32_t iX, int32_t iY, int32_t iDirection);
	int32_t FindCrawlAttach(int32_t iX, int32_t iY);
	bool IsCrawlAttach(int32_t iX, int32_t iY, int32_t iAttach);
	bool CheckBackRayShorten();
	bool Execute();
	bool CrawlTargetFree(int32_t iX, int32_t iY, int32_t iAttach, int32_t iDirection);
	bool PointFree(int32_t iX, int32_t iY);
	bool Crawl();
	bool PathFree(int32_t &rX, int32_t &rY, int32_t iToX, int32_t iToY, C4TransferZone **ppZone = nullptr);
};

C4PathFinderRay::C4PathFinderRay()
{
	Default();
}

C4PathFinderRay::~C4PathFinderRay()
{
	Clear();
}

void C4PathFinderRay::Default()
{
	Status=C4PF_Ray_Launch;
	X=Y=X2=Y2=TargetX=TargetY=0;
	Direction=0;
	Depth=0;
	UseZone=nullptr;
	From=nullptr;
	Next=nullptr;
	pPathFinder=nullptr;
	CrawlStartX=CrawlStartY=CrawlAttach=0;
}

void C4PathFinderRay::Clear()
{

}

bool C4PathFinderRay::Execute()
{
	C4TransferZone *pZone = nullptr;
	int32_t iX,iY,iLastX,iLastY;
	switch (Status)
	{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4PF_Ray_Launch:
		// In zone: use zone
		if (UseZone)
		{
			// Mark zone used
			UseZone->Used=true;
			// Target in transfer zone: success
			if (UseZone->At(TargetX,TargetY))
			{
				// Set end point
				X2=TargetX; Y2=TargetY;
				// Set complete path
				SetCompletePath();
				// Done
				pPathFinder->Success=true;
				Status=C4PF_Ray_Still;
			}
			// Continue from other end of zone
			else
			{
				// Find exit point
				if (!UseZone->GetEntryPoint(X2,Y2,TargetX,TargetY))
					{ Status=C4PF_Ray_Failure; break; }
				// Launch new ray (continue direction of entrance ray)
				if (!pPathFinder->AddRay(X2,Y2,TargetX,TargetY,Depth+1,Direction,this))
					{ Status=C4PF_Ray_Failure; break; }
				// Still
				Status=C4PF_Ray_Still;
			}
			return true;
		}
		// Not in zone: check path to target
		// Path free: success
		else if (PathFree(X2,Y2,TargetX,TargetY,&pZone))
		{
			// Set complete path
			SetCompletePath();
			// Done
			pPathFinder->Success=true;
			Status=C4PF_Ray_Still;
			return true;
		}
		// Path intersected by transfer zone
		else if (pZone)
		{
			// Zone entry point adjust (if not already in zone)
			if (!pZone->At(X,Y))
				pZone->GetEntryPoint(X2,Y2,X2,Y2);
			// Add use-zone ray
			if (!pPathFinder->AddRay(X2,Y2,TargetX,TargetY,Depth+1,Direction,this,pZone))
				{ Status=C4PF_Ray_Failure; break; }
			// Still
			Status=C4PF_Ray_Still;
			// Continue
			return true;
		}
		// Path intersected by solid
		else
		{
			// Start crawling
			Status=C4PF_Ray_Crawl;
			CrawlStartX=X2; CrawlStartY=Y2;
			CrawlAttach=FindCrawlAttach(X2,Y2);
			CrawlLength=0;
			if (!CrawlAttach) CrawlAttach=FindCrawlAttachDiagonal(X2,Y2,Direction);
			CrawlStartAttach=CrawlAttach;
			// Intersected but no attach found: unexpected failure
			if (!CrawlAttach) { Status=C4PF_Ray_Failure; break; }
			// Continue
			return true;
		}
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4PF_Ray_Crawl:
		// Crawl
		iLastX=X2; iLastY=Y2;
		if (!Crawl())
			{ Status=C4PF_Ray_Failure; break; }
		// Back at crawl starting position: done and still
		if ((X2==CrawlStartX) && (Y2==CrawlStartY) && (CrawlAttach==CrawlStartAttach))
			{ Status=C4PF_Ray_Still; break; }
		// Check unused zone intersection
		if (pPathFinder->TransferZonesEnabled)
			if (pPathFinder->TransferZones)
				if ((pZone = pPathFinder->TransferZones->Find(X2,Y2)))
					if (!pZone->Used)
					{
						// Add use-zone ray (with zone entry point adjust)
						iX=X2; iY=Y2; if (pZone->GetEntryPoint(iX,iY,X2,Y2))
							if (!pPathFinder->AddRay(iX,iY,TargetX,TargetY,Depth+1,Direction,this,pZone))
								{ Status=C4PF_Ray_Failure; break; }
						// Continue crawling
						return true;
					}
		// Crawl length
		CrawlLength++;
		if (CrawlLength >= C4PF_MaxCrawl * pPathFinder->Level)
			{ Status=C4PF_Ray_Still; break; }
		// Check back path intersection
		iX=X; iY=Y;
		if (!PathFree(iX,iY,X2,Y2))
			// Insert split ray
			if (!pPathFinder->SplitRay(this,iLastX,iLastY))
				{ Status=C4PF_Ray_Failure; break; }
		// Try new ray at target
		iX=X2; iY=Y2;
		// If has been crawling for a while
		if (CrawlLength>C4PF_Threshold)
			// If all free...
			if ( PathFree(iX,iY,TargetX,TargetY)
			     // ...or at least beyond threshold and not backwards toward crawl start
			     || ((Distance(iX,iY,X2,Y2)>C4PF_Threshold) && (Distance(iX,iY,CrawlStartX,CrawlStartY)>Distance(X2,Y2,CrawlStartX,CrawlStartY))) )
			{
				// Still
				Status=C4PF_Ray_Still;
				// Launch new rays
				if (!pPathFinder->AddRay(X2,Y2,TargetX,TargetY,Depth+1,C4PF_Direction_Left,this)
				    || !pPathFinder->AddRay(X2,Y2,TargetX,TargetY,Depth+1,C4PF_Direction_Right,this))
					{ Status=C4PF_Ray_Failure; break; }
			}
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4PF_Ray_Still: case C4PF_Ray_Failure: case C4PF_Ray_Deleted:
		return false;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	}
	return true;
}

void C4PathFinderRay::Draw(C4TargetFacet &cgo)
{
	uint32_t Color=0xffff0000;
	switch (Status)
	{
	case C4PF_Ray_Crawl: Color=C4RGB(0xff, 0, 0); break;
	case C4PF_Ray_Still: Color=C4RGB(0x50, 0, 0); break;
	case C4PF_Ray_Failure: Color=C4RGB(0xff, 0xff, 0); break;
	case C4PF_Ray_Deleted: Color=C4RGB(0x59, 0x59, 0x59); break;
	}
	if (UseZone) Color=C4RGB(0, 0, 0xff);

	// Crawl attachment
	if (Status==C4PF_Ray_Crawl)
	{
		int32_t iX=0,iY=0; CrawlToAttach(iX,iY,CrawlAttach);
		pDraw->DrawLineDw(cgo.Surface,
		                  cgo.X+X2-cgo.TargetX,cgo.Y+Y2-cgo.TargetY,
		                  cgo.X+X2-cgo.TargetX+7*iX,cgo.Y+Y2-cgo.TargetY+7*iY,
		                  C4RGB(0xff, 0, 0));
	}

	// Ray line
	pDraw->DrawLineDw(cgo.Surface,
	                  cgo.X+X-cgo.TargetX,cgo.Y+Y-cgo.TargetY,
	                  cgo.X+X2-cgo.TargetX,cgo.Y+Y2-cgo.TargetY,
	                  Color);

	// Crawler point
	pDraw->DrawFrameDw(cgo.Surface,
	                   cgo.X+X2-cgo.TargetX-1,cgo.Y+Y2-cgo.TargetY-1,
	                   cgo.X+X2-cgo.TargetX+1,cgo.Y+Y2-cgo.TargetY+1,
	                   (Status==C4PF_Ray_Crawl) ? ((Direction==C4PF_Direction_Left) ? C4RGB(0, 0xff, 0) : C4RGB(0, 0, 0xff)) : Color);

	// Search target point
	pDraw->DrawFrameDw(cgo.Surface,
	                   cgo.X+TargetX-cgo.TargetX-2,cgo.Y+TargetY-cgo.TargetY-2,
	                   cgo.X+TargetX-cgo.TargetX+2,cgo.Y+TargetY-cgo.TargetY+2,
	                   C4RGB(0xff, 0xff, 0));

}

bool C4PathFinderRay::PathFree(int32_t &rX, int32_t &rY, int32_t iToX, int32_t iToY, C4TransferZone **ppZone)
{
	int32_t d,dx,dy,aincr,bincr,xincr,yincr,x,y;
	// Y based
	if (Abs(iToX-rX)<Abs(iToY-rY))
	{
		xincr=(iToX>rX) ? +1 : -1; yincr=(iToY>rY) ? +1 : -1;
		dy=Abs(iToY-rY); dx=Abs(iToX-rX);
		d=2*dx-dy; aincr=2*(dx-dy); bincr=2*dx; x=rX; y=rY;
		for (y=rY; y!=iToY; y+=yincr)
		{
			// Check point free
			if (PointFree(x,y)) { rY=y; rX=x; }
			else return false;
			// Check transfer zone intersection
			if (ppZone)
				if (pPathFinder->TransferZonesEnabled)
					if (pPathFinder->TransferZones)
						if ((*ppZone = pPathFinder->TransferZones->Find(rX,rY)))
							return false;
			// Advance
			if (d>=0) { x+=xincr; d+=aincr; }
			else d+=bincr;
		}
	}
	// X based
	else
	{
		yincr=(iToY>rY) ? +1 : -1; xincr=(iToX>rX) ? +1 : -1;
		dx=Abs(iToX-rX); dy=Abs(iToY-rY);
		d=2*dy-dx; aincr=2*(dy-dx); bincr=2*dy; x=rX; y=rY;
		for (x=rX; x!=iToX; x+=xincr)
		{
			// Check point free
			if (PointFree(x,y)) { rY=y; rX=x; }
			else return false;
			// Check transfer zone intersection
			if (ppZone)
				if (pPathFinder->TransferZonesEnabled)
					if (pPathFinder->TransferZones)
						if ((*ppZone = pPathFinder->TransferZones->Find(rX,rY)))
							return false;
			// Advance
			if (d>=0) { y+=yincr; d+=aincr; }
			else d+=bincr;
		}
	}

	return true;
}

bool C4PathFinderRay::Crawl()
{

	// No attach: crawl failure (shouldn't ever get here)
	if (!CrawlAttach)
		return false;

	// Last attach lost (don't check on first crawl for that might be a diagonal attach)
	if (CrawlLength)
		if (!IsCrawlAttach(X2,Y2,CrawlAttach))
		{
			// Crawl corner by last attach
			CrawlToAttach(X2,Y2,CrawlAttach);
			TurnAttach(CrawlAttach,-Direction);
			// Safety: new attach not found - unexpected failure
			if (!IsCrawlAttach(X2,Y2,CrawlAttach))
				return false;
			// Corner okay
			return true;
		}

	// Check crawl target by attach
	int32_t iTurned=0;
	while (!CrawlTargetFree(X2,Y2,CrawlAttach,Direction))
	{
		// Crawl target not free: turn attach
		TurnAttach(CrawlAttach,Direction); iTurned++;
		// Turned four times: all enclosed, crawl failure
		if (iTurned==4)
			return false;
	}

	// Crawl by attach
	CrawlByAttach(X2,Y2,CrawlAttach,Direction);

	// Success
	return true;

}

void C4PathFinderRay::SetCompletePath()
{
	C4PathFinderRay *pRay;
	// Back shorten
	for (pRay=this; pRay->From; pRay=pRay->From)
		while (pRay->CheckBackRayShorten()) {}
	// Set all waypoints
	for (pRay=this; pRay->From; pRay=pRay->From)
	{
		// Transfer waypoint
		if (pRay->UseZone)
			pPathFinder->SetWaypoint(pRay->X2,pRay->Y2,pRay->UseZone->Object);
		// MoveTo waypoint
		else
			pPathFinder->SetWaypoint(pRay->From->X2,pRay->From->Y2,nullptr);
	}
}

bool C4PathFinderRay::PointFree(int32_t iX, int32_t iY)
{
	return pPathFinder->PointFree(iX,iY);
}

bool C4PathFinderRay::CrawlTargetFree(int32_t iX, int32_t iY, int32_t iAttach, int32_t iDirection)
{
	CrawlByAttach(iX,iY,iAttach,iDirection);
	return PointFree(iX,iY);
}

void C4PathFinderRay::CrawlByAttach(int32_t &rX, int32_t &rY, int32_t iAttach, int32_t iDirection)
{
	switch (iAttach)
	{
	case C4PF_Crawl_Top: rX+=iDirection; break;
	case C4PF_Crawl_Bottom: rX-=iDirection; break;
	case C4PF_Crawl_Left: rY-=iDirection; break;
	case C4PF_Crawl_Right: rY+=iDirection; break;
	}
}

void C4PathFinderRay::TurnAttach(int32_t &rAttach, int32_t iDirection)
{
	rAttach+=iDirection;
	if (rAttach>C4PF_Crawl_Left) rAttach=C4PF_Crawl_Top;
	if (rAttach<C4PF_Crawl_Top) rAttach=C4PF_Crawl_Left;
}

int32_t C4PathFinderRay::FindCrawlAttach(int32_t iX, int32_t iY)
{
	if (!PointFree(iX,iY-1)) return C4PF_Crawl_Top;
	if (!PointFree(iX,iY+1)) return C4PF_Crawl_Bottom;
	if (!PointFree(iX-1,iY)) return C4PF_Crawl_Left;
	if (!PointFree(iX+1,iY)) return C4PF_Crawl_Right;
	return C4PF_Crawl_NoAttach;
}

void C4PathFinderRay::CrawlToAttach(int32_t &rX, int32_t &rY, int32_t iAttach)
{
	switch (iAttach)
	{
	case C4PF_Crawl_Top: rY--; break;
	case C4PF_Crawl_Bottom: rY++; break;
	case C4PF_Crawl_Left: rX--; break;
	case C4PF_Crawl_Right: rX++; break;
	}
}

bool C4PathFinderRay::IsCrawlAttach(int32_t iX, int32_t iY, int32_t iAttach)
{
	CrawlToAttach(iX,iY,iAttach);
	return !PointFree(iX,iY);
}

int32_t C4PathFinderRay::FindCrawlAttachDiagonal(int32_t iX, int32_t iY, int32_t iDirection)
{
	// Going left
	if (iDirection==C4PF_Direction_Left)
	{
		// Top Left
		if (!PointFree(iX-1,iY-1)) return C4PF_Crawl_Top;
		// Bottom left
		if (!PointFree(iX-1,iY+1)) return C4PF_Crawl_Left;
		// Top right
		if (!PointFree(iX+1,iY-1)) return C4PF_Crawl_Right;
		// Bottom right
		if (!PointFree(iX+1,iY+1)) return C4PF_Crawl_Bottom;
	}
	// Going right
	if (iDirection==C4PF_Direction_Right)
	{
		// Top Left
		if (!PointFree(iX-1,iY-1)) return C4PF_Crawl_Left;
		// Bottom left
		if (!PointFree(iX-1,iY+1)) return C4PF_Crawl_Bottom;
		// Top right
		if (!PointFree(iX+1,iY-1)) return C4PF_Crawl_Top;
		// Bottom right
		if (!PointFree(iX+1,iY+1)) return C4PF_Crawl_Right;
	}
	return C4PF_Crawl_NoAttach;
}

bool C4PathFinderRay::CheckBackRayShorten()
{
	C4PathFinderRay *pRay,*pRay2;
	int32_t iX,iY;
	for (pRay=From; pRay; pRay=pRay->From)
	{
		// Don't shorten transfer over zones
		if (pRay->UseZone) return false;
		// Skip self
		if (pRay==From) continue;
		// Check shortcut
		iX=X; iY=Y;
		if (PathFree(iX,iY,pRay->X,pRay->Y))
		{
			// Delete jumped rays
			for (pRay2=From; pRay2!=pRay; pRay2=pRay2->From)
				pRay2->Status=C4PF_Ray_Deleted;
			// Shorten pRay to this
			pRay->X2=X; pRay->Y2=Y;
			From=pRay;
			// Success
			return true;
		}
	}
	return false;
}

//------------------------------- C4PathFinder ---------------------------------------------

C4PathFinder::C4PathFinder()
{
	Default();
}

C4PathFinder::~C4PathFinder()
{
	Clear();
}

void C4PathFinder::Default()
{
	PointFree=nullptr;
	SetWaypoint=nullptr;
	FirstRay=nullptr;
	Success=false;
	TransferZones=nullptr;
	TransferZonesEnabled=true;
	Level=1;
}

void C4PathFinder::Clear()
{
	C4PathFinderRay *pRay,*pNext;
	for (pRay=FirstRay; pRay; pRay=pNext) { pNext=pRay->Next; delete pRay; }
	FirstRay=nullptr;
}

void C4PathFinder::Init(PointFreeFn fnPointFree, C4TransferZones* pTransferZones)
{
	// Set data
	PointFree = fnPointFree;
	TransferZones = pTransferZones;
}

void C4PathFinder::EnableTransferZones(bool fEnabled)
{
	TransferZonesEnabled = fEnabled;
}

void C4PathFinder::SetLevel(int iLevel)
{
	Level = Clamp(iLevel, 1, 10);
}

void C4PathFinder::Draw(C4TargetFacet &cgo)
{
	if (TransferZones) TransferZones->Draw(cgo);
	for (C4PathFinderRay *pRay=FirstRay; pRay; pRay=pRay->Next) pRay->Draw(cgo);
}

void C4PathFinder::Run()
{
	if (TransferZones) TransferZones->ClearUsed();
	Success=false;
	while (!Success && Execute()) {}
	// Notice that ray zone-pointers might be invalid after run
}

bool C4PathFinder::Execute()
{

	// Execute & count rays
	bool fContinue=false;
	int32_t iRays=0;
	for (C4PathFinderRay *pRay=FirstRay; pRay && !Success; pRay=pRay->Next,iRays++)
		if (pRay->Execute())
			fContinue=true;

	// Max ray limit
	if (iRays>=C4PF_MaxRay) return false;

	// Draw
	if (::GraphicsSystem.ShowPathfinder)
	{
		static int32_t iDelay=0;
		iDelay++; if (iDelay>C4PF_Draw_Rate)
		{
			iDelay=0;
			::GraphicsSystem.Execute();
		}
	}

	return fContinue;
}

bool C4PathFinder::Find(int32_t iFromX, int32_t iFromY, int32_t iToX, int32_t iToY, SetWaypointFn fnSetWaypoint)
{

	// Prepare
	Clear();

	// Parameter safety
	if (!fnSetWaypoint) return false;
	SetWaypoint=fnSetWaypoint;

	// Start & target coordinates must be free
	if (!PointFree(iFromX,iFromY) || !PointFree(iToX,iToY)) return false;

	// Add the first two rays
	if (!AddRay(iFromX,iFromY,iToX,iToY,0,C4PF_Direction_Left,nullptr)) return false;
	if (!AddRay(iFromX,iFromY,iToX,iToY,0,C4PF_Direction_Right,nullptr)) return false;

	// Run
	Run();

	// Success
	return Success;

}

bool C4PathFinder::AddRay(int32_t iFromX, int32_t iFromY, int32_t iToX, int32_t iToY, int32_t iDepth, int32_t iDirection, C4PathFinderRay *pFrom, C4TransferZone *pUseZone)
{
	// Max depth
	if (iDepth >= C4PF_MaxDepth * Level) return false;
	// Allocate and set new ray
	C4PathFinderRay *pRay;
	if (!(pRay = new C4PathFinderRay)) return false;
	pRay->X=iFromX; pRay->Y=iFromY;
	pRay->X2=iFromX; pRay->Y2=iFromY;
	pRay->TargetX=iToX; pRay->TargetY=iToY;
	pRay->Depth=iDepth;
	pRay->Direction=iDirection;
	pRay->From=pFrom;
	pRay->pPathFinder=this;
	pRay->Next=FirstRay;
	pRay->UseZone=pUseZone;
	FirstRay=pRay;
	return true;
}

bool C4PathFinder::SplitRay(C4PathFinderRay *pRay, int32_t iAtX, int32_t iAtY)
{
	// Max depth
	if (pRay->Depth >= C4PF_MaxDepth * Level) return false;
	// Allocate and set new ray
	C4PathFinderRay *pNewRay;
	if (!(pNewRay = new C4PathFinderRay)) return false;
	pNewRay->Status=C4PF_Ray_Still;
	pNewRay->X=pRay->X; pNewRay->Y=pRay->Y;
	pNewRay->X2=iAtX; pNewRay->Y2=iAtY;
	pNewRay->TargetX=pRay->TargetX; pNewRay->TargetY=pRay->TargetY;
	pNewRay->Depth=pRay->Depth;
	pNewRay->Direction=pRay->Direction;
	pNewRay->From=pRay->From;
	pNewRay->pPathFinder=this;
	pNewRay->Next=FirstRay;
	FirstRay=pNewRay;
	// Adjust split ray
	pRay->From=pNewRay;
	pRay->X=iAtX; pRay->Y=iAtY;
	return true;
}


