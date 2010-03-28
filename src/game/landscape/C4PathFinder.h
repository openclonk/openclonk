/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001, 2005  Sven Eberhardt
 * Copyright (c) 2005  Armin Burgmeier
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */

/* Finds the way through the Clonk landscape */

#ifndef INC_C4PathFinder
#define INC_C4PathFinder

#include <C4TransferZone.h>

class C4PathFinder;

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
protected:
	bool (*PointFree)(int32_t, int32_t);
	// iToX and iToY are intptr_t because there are stored object
	// pointers sometimes
	bool (*SetWaypoint)(int32_t, int32_t, intptr_t, intptr_t);
	C4PathFinderRay *FirstRay;
	intptr_t WaypointParameter;
	bool Success;
	C4TransferZones *TransferZones;
	bool TransferZonesEnabled;
	int Level;
public:
	void Draw(C4TargetFacet &cgo);
	void Clear();
	void Default();
	void Init(bool (*fnPointFree)(int32_t, int32_t), C4TransferZones* pTransferZones=NULL);
	bool Find(int32_t iFromX, int32_t iFromY, int32_t iToX, int32_t iToY, bool (*fnSetWaypoint)(int32_t, int32_t, intptr_t, intptr_t), intptr_t iWaypointParameter);
	void EnableTransferZones(bool fEnabled);
	void SetLevel(int iLevel);
protected:
	void Run();
	bool AddRay(int32_t iFromX, int32_t iFromY, int32_t iToX, int32_t iToY, int32_t iDepth, int32_t iDirection, C4PathFinderRay *pFrom, C4TransferZone *pUseZone=NULL);
	bool SplitRay(C4PathFinderRay *pRay, int32_t iAtX, int32_t iAtY);
	bool Execute();
};


#endif
