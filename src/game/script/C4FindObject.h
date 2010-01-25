/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2006-2007  Peter Wortmann
 * Copyright (c) 2006, 2008  Günther Brammer
 * Copyright (c) 2007  Sven Eberhardt
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
#ifndef C4FINDOBJECT_H
#define C4FINDOBJECT_H

#include "C4Id.h"
#include "C4Shape.h"
#include "C4Value.h"
#include "C4Aul.h"

// Condition map
enum C4FindObjectCondID
{
	C4FO_Not           = 1,
	C4FO_And           = 2,
	C4FO_Or            = 3,
	C4FO_Exclude       = 5,
	C4FO_InRect        = 10,
	C4FO_AtPoint       = 11,
	C4FO_AtRect        = 12,
	C4FO_OnLine        = 13,
	C4FO_Distance      = 14,
	C4FO_ID            = 20,
	C4FO_OCF           = 21,
	C4FO_Category      = 22,
	C4FO_Action        = 30,
	C4FO_ActionTarget  = 31,
	C4FO_Procedure     = 32,
	C4FO_Container     = 40,
	C4FO_AnyContainer  = 41,
	C4FO_Owner         = 50,
	C4FO_Controller    = 51,
	C4FO_Func          = 60,
	C4FO_Layer         = 70
};

// Sort map - using same values as C4FindObjectCondID!
enum C4SortObjectCondID
	{
	C4SO_First        = 100, // no sort condition smaller than this
	C4SO_Reverse      = 101, // reverse sort order
	C4SO_Multiple     = 102, // multiple sorts; high priority first; lower priorities if higher prio returned equal
	C4SO_Distance     = 110, // nearest first
	C4SO_Random       = 120, // random first
	C4SO_Speed        = 130, // slowest first
	C4SO_Mass         = 140, // lightest first
	C4SO_Value        = 150, // cheapest first
	C4SO_Func         = 160, // least return values first
	C4SO_Last         = 200  // no sort condition larger than this
	};

class C4LSectors;
class C4ObjectList;

// Base class
class C4FindObject
{
	friend class C4FindObjectNot;
	friend class C4FindObjectAnd;
	friend class C4FindObjectOr;

	class C4SortObject *pSort;
public:
	C4FindObject() : pSort(NULL) { }
	virtual ~C4FindObject();

	static C4FindObject *CreateByValue(const C4Value &Data, C4SortObject **ppSortObj=NULL); // createFindObject or SortObject - if ppSortObj==NULL, SortObject is not allowed

	int32_t Count(const C4ObjectList &Objs); // Counts objects for which the condition is true
	C4Object *Find(const C4ObjectList &Objs);	// Returns first object for which the condition is true
	C4ValueArray *FindMany(const C4ObjectList &Objs); // Returns all objects for which the condition is true

	int32_t Count(const C4ObjectList &Objs, const C4LSectors &Sct); // Counts objects for which the condition is true
	C4Object *Find(const C4ObjectList &Objs, const C4LSectors &Sct);	// Returns first object for which the condition is true
	C4ValueArray *FindMany(const C4ObjectList &Objs, const C4LSectors &Sct); // Returns all objects for which the condition is true

	void SetSort(C4SortObject *pToSort);

protected:
	// Overridables
	virtual bool Check(C4Object *pObj) = 0;
	virtual C4Rect *GetBounds() { return NULL; }
	virtual bool UseShapes() { return false; }
	virtual bool IsImpossible() { return false; }
	virtual bool IsEnsured() { return false; }

private:
	void CheckObjectStatus(C4ValueArray *pArray);
};

// Combinators
class C4FindObjectNot : public C4FindObject
{
public:
	C4FindObjectNot(C4FindObject *pCond)
		: pCond(pCond) { }
	virtual ~C4FindObjectNot();
private:
	C4FindObject *pCond;
protected:
	virtual bool Check(C4Object *pObj);
	virtual bool IsImpossible() { return pCond->IsEnsured(); }
	virtual bool IsEnsured() { return pCond->IsImpossible(); }
};

class C4FindObjectAnd : public C4FindObject
{
public:
	C4FindObjectAnd(int32_t iCnt, C4FindObject **ppConds, bool fFreeArray = true);
	virtual ~C4FindObjectAnd();
private:
	int32_t iCnt;
	C4FindObject **ppConds; bool fFreeArray; bool fUseShapes;
	C4Rect Bounds; bool fHasBounds;
protected:
	virtual bool Check(C4Object *pObj);
	virtual C4Rect *GetBounds() { return fHasBounds ? &Bounds : NULL; }
	virtual bool UseShapes() { return fUseShapes; }
	virtual bool IsEnsured() { return !iCnt; }
	virtual bool IsImpossible();
};

class C4FindObjectOr : public C4FindObject
{
public:
	C4FindObjectOr(int32_t iCnt, C4FindObject **ppConds);
	virtual ~C4FindObjectOr();
private:
	int32_t iCnt;
	C4FindObject **ppConds;
	C4Rect Bounds; bool fHasBounds;
protected:
	virtual bool Check(C4Object *pObj);
	virtual C4Rect *GetBounds() { return fHasBounds ? &Bounds : NULL; }
	virtual bool IsEnsured();
	virtual bool IsImpossible() { return !iCnt; }
};

// Primitive conditions
class C4FindObjectExclude : public C4FindObject
{
public:
	C4FindObjectExclude(C4Object *pExclude)
		: pExclude(pExclude) { }
private:
	C4Object *pExclude;
protected:
	virtual bool Check(C4Object *pObj);
};

class C4FindObjectID : public C4FindObject
{
public:
	C4FindObjectID(C4ID id)
		: id(id) { }
private:
	C4ID id;
protected:
	virtual bool Check(C4Object *pObj);
	virtual bool IsImpossible();
};

class C4FindObjectInRect : public C4FindObject
{
public:
	C4FindObjectInRect(const C4Rect &rect)
		: rect(rect) { }
private:
	C4Rect rect;
protected:
	virtual bool Check(C4Object *pObj);
	virtual C4Rect *GetBounds() { return &rect; }
	virtual bool IsImpossible();
};

class C4FindObjectAtPoint : public C4FindObject
{
public:
	C4FindObjectAtPoint(int32_t x, int32_t y)
		: bounds(x, y, 1, 1) { }
private:
	C4Rect bounds;
protected:
	virtual bool Check(C4Object *pObj);
	virtual C4Rect *GetBounds() { return &bounds; }
	virtual bool UseShapes() { return true; }
};

class C4FindObjectAtRect : public C4FindObject
{
public:
	C4FindObjectAtRect(int32_t x, int32_t y, int32_t wdt, int32_t hgt)
		: bounds(x, y, wdt, hgt) { }
private:
	C4Rect bounds;
protected:
	virtual bool Check(C4Object *pObj);
	virtual C4Rect *GetBounds() { return &bounds; }
	virtual bool UseShapes() { return true; }
};

class C4FindObjectOnLine : public C4FindObject
{
public:
	C4FindObjectOnLine(int32_t x, int32_t y, int32_t x2, int32_t y2)
		: x(x), y(y), x2(x2), y2(y2), bounds(x, y, 1, 1) { bounds.Add(C4Rect(x2, y2, 1,1)); }
private:
	int32_t x, y, x2, y2;
	C4Rect bounds;
protected:
	virtual bool Check(C4Object *pObj);
	virtual C4Rect *GetBounds() { return &bounds; }
	virtual bool UseShapes() { return true; }
};

class C4FindObjectDistance : public C4FindObject
{
public:
	C4FindObjectDistance(int32_t x, int32_t y, int32_t r)
		: x(x), y(y), r2(r*r), bounds(x-r, y-r, 2*r+1, 2*r+1) { }
private:
	int32_t x, y, r2;
	C4Rect bounds;
protected:
	virtual bool Check(C4Object *pObj);
	virtual C4Rect *GetBounds() { return &bounds; }
};

class C4FindObjectOCF : public C4FindObject
{
public:
	C4FindObjectOCF(int32_t ocf)
		: ocf(ocf) { }
private:
	int32_t ocf;
protected:
	virtual bool Check(C4Object *pObj);
	virtual bool IsImpossible();
};

class C4FindObjectCategory : public C4FindObject
{
public:
	C4FindObjectCategory(int32_t iCategory)
		: iCategory(iCategory) { }
private:
	int32_t iCategory;
protected:
	virtual bool Check(C4Object *pObj);
	virtual bool IsEnsured();
};

class C4FindObjectAction : public C4FindObject
{
public:
	C4FindObjectAction(const char *szAction)
		: szAction(szAction) { }
private:
	const char *szAction;
protected:
	virtual bool Check(C4Object *pObj);
};

class C4FindObjectActionTarget : public C4FindObject
{
public:
	C4FindObjectActionTarget(C4Object *pActionTarget, int index)
		: pActionTarget(pActionTarget), index(index) { }
private:
	C4Object *pActionTarget;
	int index;
protected:
	virtual bool Check(C4Object *pObj);
};

class C4FindObjectProcedure : public C4FindObject
{
public:
	C4FindObjectProcedure(int32_t procedure)
		: procedure(procedure) { }
private:
	int32_t procedure;
protected:
	virtual bool Check(C4Object *pObj);
	virtual bool IsImpossible();
};

class C4FindObjectContainer : public C4FindObject
{
public:
	C4FindObjectContainer(C4Object *pContainer)
		: pContainer(pContainer) { }
private:
	C4Object *pContainer;
protected:
	virtual bool Check(C4Object *pObj);
};

class C4FindObjectAnyContainer : public C4FindObject
{
public:
	C4FindObjectAnyContainer() { }
protected:
	virtual bool Check(C4Object *pObj);
};

class C4FindObjectOwner : public C4FindObject
{
public:
	C4FindObjectOwner(int32_t iOwner)
		: iOwner(iOwner) { }
private:
	int32_t iOwner;
protected:
	virtual bool Check(C4Object *pObj);
	virtual bool IsImpossible();
};

class C4FindObjectController : public C4FindObject
{
public:
	C4FindObjectController(int32_t controller)
		: controller(controller) { }
private:
	int32_t controller;
protected:
	virtual bool Check(C4Object *pObj);
	virtual bool IsImpossible();
};

class C4FindObjectFunc : public C4FindObject
{
public:
	C4FindObjectFunc(const char *Name): Name(Name) { }
	void SetPar(int i, const C4Value &val);
private:
	const char * Name;
	C4AulParSet Pars;
protected:
	virtual bool Check(C4Object *pObj);
	virtual bool IsImpossible();
};

class C4FindObjectLayer : public C4FindObject
{
public:
	C4FindObjectLayer(C4Object *pLayer) : pLayer(pLayer) {}
private:
	C4Object *pLayer;
protected:
	virtual bool Check(C4Object *pObj);
	virtual bool IsImpossible();
};

// result sorting
class C4SortObject
	{
	public:
		C4SortObject() {}
		virtual ~C4SortObject() {}

	public:
		// Overridables
		virtual int32_t Compare(C4Object *pObj1, C4Object *pObj2) = 0; // return value <0 if obj1 is to be sorted before obj2

		virtual bool PrepareCache(const C4ValueArray *pObjs) { return false; }
		virtual int32_t CompareCache(int32_t iObj1, int32_t iObj2, C4Object *pObj1, C4Object *pObj2) { return Compare(pObj1, pObj2); }

	public:
		static C4SortObject *CreateByValue(const C4Value &Data);
		static C4SortObject *CreateByValue(int32_t iType, const C4ValueArray &Data);

		void SortObjects(C4ValueArray *pArray);
	};

class C4SortObjectByValue : public C4SortObject
	{
	public:
		C4SortObjectByValue();
		virtual ~C4SortObjectByValue();

	private:
		int32_t *pVals;
		int32_t iSize;

	public:
		// Overridables
		virtual int32_t Compare(C4Object *pObj1, C4Object *pObj2);
		virtual int32_t CompareGetValue(C4Object *pOf) = 0;

		virtual bool PrepareCache(const C4ValueArray *pObjs);
		virtual int32_t CompareCache(int32_t iObj1, int32_t iObj2, C4Object *pObj1, C4Object *pObj2);

	};

class C4SortObjectReverse : public C4SortObject // reverse sort
	{
	public:
		C4SortObjectReverse(C4SortObject *pSort)
			: C4SortObject(), pSort(pSort) {}
		virtual ~C4SortObjectReverse();
	private:
		C4SortObject *pSort;

	protected:
		int32_t Compare(C4Object *pObj1, C4Object *pObj2);

		virtual bool PrepareCache(const C4ValueArray *pObjs);
		virtual int32_t CompareCache(int32_t iObj1, int32_t iObj2, C4Object *pObj1, C4Object *pObj2);
	};

class C4SortObjectMultiple : public C4SortObject // apply next sort if previous compares to equality
	{
	public:
		C4SortObjectMultiple(int32_t iCnt, C4SortObject **ppSorts, bool fFreeArray = true)
			: C4SortObject(), fFreeArray(fFreeArray), iCnt(iCnt), ppSorts(ppSorts) {}
		virtual ~C4SortObjectMultiple();
	private:
		bool fFreeArray;
		int32_t iCnt;
		C4SortObject **ppSorts;

	protected:
		int32_t Compare(C4Object *pObj1, C4Object *pObj2);

		virtual bool PrepareCache(const C4ValueArray *pObjs);
		virtual int32_t CompareCache(int32_t iObj1, int32_t iObj2, C4Object *pObj1, C4Object *pObj2);
	};

class C4SortObjectDistance : public C4SortObjectByValue // sort by distance from point x/y
	{
	public:
		C4SortObjectDistance(int iX, int iY)
			: C4SortObjectByValue(), iX(iX), iY(iY) {}
	private:
		int iX, iY;

	protected:
		int32_t CompareGetValue(C4Object *pFor);
	};

class C4SortObjectRandom : public C4SortObjectByValue // randomize order
	{
	public:
		C4SortObjectRandom() : C4SortObjectByValue(){}

	protected:
		int32_t CompareGetValue(C4Object *pFor);
	};

class C4SortObjectSpeed : public C4SortObjectByValue // sort by object xdir/ydir
	{
	public:
		C4SortObjectSpeed() : C4SortObjectByValue(){}

	protected:
		int32_t CompareGetValue(C4Object *pFor);
	};

class C4SortObjectMass : public C4SortObjectByValue // sort by mass
	{
	public:
		C4SortObjectMass() : C4SortObjectByValue(){}

	protected:
		int32_t CompareGetValue(C4Object *pFor);
	};

class C4SortObjectValue : public C4SortObjectByValue // sort by value
	{
	public:
		C4SortObjectValue() : C4SortObjectByValue(){}

	protected:
		int32_t CompareGetValue(C4Object *pFor);
	};

class C4SortObjectFunc : public C4SortObjectByValue // sort by script function
	{
	public:
		C4SortObjectFunc(const char * Name): Name(Name) { }
		void SetPar(int i, const C4Value &val);
	private:
		const char * Name;
		C4AulParSet Pars;
	protected:
		int32_t CompareGetValue(C4Object *pFor);
	};

#endif
