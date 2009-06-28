/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
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

/* Finds the shortest free path through any pixel environment */

/* I am rather proud of this class. If you are going to use it,
   please give me the credits. */

class CPathFinder;

class CPathFinderRay
	{
	friend class CPathFinder;
	public:
		CPathFinderRay();
		~CPathFinderRay();
	public:
		void Clear();
		void Default();
	protected:
		int Status;
		int X,Y,X2,Y2,TargetX,TargetY;
		int CrawlStartX,CrawlStartY,CrawlAttach,CrawlLength,CrawlStartAttach;
		int Direction,Depth;
		CPathFinderRay *From;
		CPathFinderRay *Next;
		CPathFinder *pPathFinder;
	protected:
		BOOL IsCrawlAttach(int iX, int iY, int iAttach);
		BOOL CheckBackRayShorten();
		int FindCrawlAttachDiagonal(int iX, int iY, int iDirection);
		int FindCrawlAttach(int iX, int iY);
		void TurnAttach(int &rAttach, int iDirection);
		void CrawlToAttach(int &rX, int &rY, int iAttach);
		void CrawlByAttach(int &rX, int &rY, int iAttach, int iDirection);
		BOOL CrawlTargetFree(int iX, int iY, int iAttach, int iDirection);
		BOOL PointFree(int iX, int iY);
		BOOL Crawl();
		BOOL PathFree(int &rX, int &rY, int iToX, int iToY);
		BOOL Execute();
	};

class CPathFinder
	{
	friend class CPathFinderRay;
	public:
		CPathFinder();
		~CPathFinder();
	protected:
		BOOL Success;
		BOOL (*PointFree)(int, int, int);
		BOOL (*SetWaypoint)(int, int, int);
		CPathFinderRay *FirstRay;
		int WaypointParameter;
		int PointFreeParameter;
		int MaxDepth;
		int MaxCrawl;
		int MaxRay;
		int Threshold;
	public:
		void Clear();
		void Default();
		void Init(BOOL (*fnPointFree)(int, int, int), int iPointFreeParameter, int iDepth=50, int iCrawl=1000, int iRay=500, int iThreshold=10);
		BOOL Find(int iFromX, int iFromY, int iToX, int iToY, BOOL (*fnSetWaypoint)(int, int, int), int iWaypointParameter);
	protected:
		BOOL SplitRay(CPathFinderRay *pRay, int iAtX, int iAtY);
		BOOL AddRay(int iFromX, int iFromY, int iToX, int iToY, int iDepth, int iDirection, CPathFinderRay *pFrom);
		BOOL Execute();
		void Run();
	};



