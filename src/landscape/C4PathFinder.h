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

#ifndef INC_C4PathFinder
#define INC_C4PathFinder

#include <functional>

class C4Object;
class C4PathFinderRay;
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
	void Init(PointFreeFn fnPointFree, C4TransferZones* pTransferZones=nullptr);
	bool Find(int32_t iFromX, int32_t iFromY, int32_t iToX, int32_t iToY, SetWaypointFn fnSetWaypoint);
	void EnableTransferZones(bool fEnabled);
	void SetLevel(int iLevel);

private:
	void Run();
	bool AddRay(int32_t iFromX, int32_t iFromY, int32_t iToX, int32_t iToY, int32_t iDepth, int32_t iDirection, C4PathFinderRay *pFrom, C4TransferZone *pUseZone=nullptr);
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
