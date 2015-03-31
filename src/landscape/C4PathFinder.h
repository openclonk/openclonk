/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2015, The OpenClonk Team and contributors
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

#ifndef INC_C4PathFinder
#define INC_C4PathFinder

#include <functional>

class C4PathFinder;
class C4Object;

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
	int32_t X,Y,X2,Y2,TargetX,TargetY;
	int32_t CrawlStartX,CrawlStartY,CrawlAttach,CrawlLength,CrawlStartAttach;
	int32_t Direction,Depth;
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
	bool PathFree(int32_t &rX, int32_t &rY, int32_t iToX, int32_t iToY, C4TransferZone **ppZone=NULL);
};

class C4PathFinder
{
	friend class C4PathFinderRay;
public:
	C4PathFinder();
	~C4PathFinder();

	typedef std::function<bool(int32_t x, int32_t y)> PointFreeFn;
	typedef std::function<bool(int32_t x, int32_t y, C4Object *transfer_object)> SetWaypointFn;

	void Draw(C4TargetFacet &cgo);
	void Clear();
	void Default();
	void Init(PointFreeFn fnPointFree, C4TransferZones* pTransferZones=NULL);
	bool Find(int32_t iFromX, int32_t iFromY, int32_t iToX, int32_t iToY, SetWaypointFn fnSetWaypoint);
	void EnableTransferZones(bool fEnabled);
	void SetLevel(int iLevel);

private:
	void Run();
	bool AddRay(int32_t iFromX, int32_t iFromY, int32_t iToX, int32_t iToY, int32_t iDepth, int32_t iDirection, C4PathFinderRay *pFrom, C4TransferZone *pUseZone=NULL);
	bool SplitRay(C4PathFinderRay *pRay, int32_t iAtX, int32_t iAtY);
	bool Execute();

	PointFreeFn PointFree;
	SetWaypointFn SetWaypoint;
	C4PathFinderRay *FirstRay;
	bool Success;
	C4TransferZones *TransferZones;
	bool TransferZonesEnabled;
	int Level;
};


#endif
