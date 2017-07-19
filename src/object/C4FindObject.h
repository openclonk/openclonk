/*
 * OpenClonk, http://www.openclonk.org
 *
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
#ifndef C4FINDOBJECT_H
#define C4FINDOBJECT_H

#include "lib/C4Rect.h"
#include "script/C4Value.h"

// Condition map
enum C4FindObjectCondID
{
	C4FO_Not           = 1,
	C4FO_And           = 2,
	C4FO_Or            = 3,
	C4FO_Exclude       = 4,
	C4FO_InRect        = 5,
	C4FO_AtPoint       = 6,
	C4FO_AtRect        = 7,
	C4FO_OnLine        = 8,
	C4FO_Distance      = 9,
	C4FO_ID            = 10,
	C4FO_OCF           = 11,
	C4FO_Category      = 12,
	C4FO_Action        = 13,
	C4FO_ActionTarget  = 14,
	C4FO_Procedure     = 15,
	C4FO_Container     = 16,
	C4FO_AnyContainer  = 17,
	C4FO_Owner         = 18,
	C4FO_Controller    = 19,
	C4FO_Func          = 20,
	C4FO_Layer         = 21,
	C4FO_InArray       = 22,
	C4FO_Property      = 23,
	C4FO_AnyLayer      = 24,
	C4FO_Cone          = 25,
	// last C4FO must be smaller than C4SO_First.
};

// Sort map - using same values as C4FindObjectCondID!
enum C4SortObjectCondID
{
	C4SO_First        = 30, // no sort condition smaller than this
	C4SO_Reverse      = 31, // reverse sort order
	C4SO_Multiple     = 32, // multiple sorts; high priority first; lower priorities if higher prio returned equal
	C4SO_Distance     = 33, // nearest first
	C4SO_Random       = 34, // random first
	C4SO_Speed        = 35, // slowest first
	C4SO_Mass         = 36, // lightest first
	C4SO_Value        = 37, // cheapest first
	C4SO_Func         = 38, // least return values first
	C4SO_Last         = 50  // no sort condition larger than this
};

// Base class
class C4FindObject
{
	friend class C4FindObjectNot;
	friend class C4FindObjectAnd;
	friend class C4FindObjectOr;

	class C4SortObject *pSort{nullptr};
public:
	C4FindObject() = default;
	virtual ~C4FindObject();

	static C4FindObject *CreateByValue(const C4Value &Data, C4SortObject **ppSortObj=nullptr, const C4Object *context=nullptr, bool *has_layer_check=nullptr); // createFindObject or SortObject - if ppSortObj==nullptr, SortObject is not allowed

	int32_t Count(const C4ObjectList &Objs); // Counts objects for which the condition is true
	C4Object *Find(const C4ObjectList &Objs); // Returns first object for which the condition is true
	C4ValueArray *FindMany(const C4ObjectList &Objs); // Returns all objects for which the condition is true

	int32_t Count(const C4ObjectList &Objs, const C4LSectors &Sct); // Counts objects for which the condition is true
	C4Object *Find(const C4ObjectList &Objs, const C4LSectors &Sct);  // Returns first object for which the condition is true
	C4ValueArray *FindMany(const C4ObjectList &Objs, const C4LSectors &Sct); // Returns all objects for which the condition is true

	void SetSort(C4SortObject *pToSort);

protected:
	// Overridables
	virtual bool Check(C4Object *pObj) = 0;
	virtual C4Rect *GetBounds() { return nullptr; }
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
	~C4FindObjectNot() override;
private:
	C4FindObject *pCond;
protected:
	bool Check(C4Object *pObj) override;
	bool IsImpossible() override { return pCond->IsEnsured(); }
	bool IsEnsured() override { return pCond->IsImpossible(); }
};

class C4FindObjectAnd : public C4FindObject
{
public:
	C4FindObjectAnd(int32_t iCnt, C4FindObject **ppConds, bool fFreeArray = true);
	~C4FindObjectAnd() override;
private:
	int32_t iCnt;
	C4FindObject **ppConds; bool fFreeArray; bool fUseShapes;
	C4Rect Bounds; bool fHasBounds;
protected:
	bool Check(C4Object *pObj) override;
	C4Rect *GetBounds() override { return fHasBounds ? &Bounds : nullptr; }
	bool UseShapes() override { return fUseShapes; }
	bool IsEnsured() override { return !iCnt; }
	bool IsImpossible() override;
	void ForgetConditions() { ppConds=nullptr; iCnt=0; }
};

// Special variant of C4FindObjectAnd that does not free its conditions
class C4FindObjectAndStatic : public C4FindObjectAnd
{
public:
	C4FindObjectAndStatic(int32_t iCnt, C4FindObject **ppConds)
		: C4FindObjectAnd(iCnt, ppConds, true) {}
	~C4FindObjectAndStatic() override {ForgetConditions(); }
};

class C4FindObjectOr : public C4FindObject
{
public:
	C4FindObjectOr(int32_t iCnt, C4FindObject **ppConds);
	~C4FindObjectOr() override;
private:
	int32_t iCnt;
	C4FindObject **ppConds; bool fUseShapes;
	C4Rect Bounds; bool fHasBounds;
protected:
	bool Check(C4Object *pObj) override;
	C4Rect *GetBounds() override { return fHasBounds ? &Bounds : nullptr; }
	bool UseShapes() override { return fUseShapes; }
	bool IsEnsured() override;
	bool IsImpossible() override { return !iCnt; }
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
	bool Check(C4Object *pObj) override;
};

class C4FindObjectDef : public C4FindObject
{
public:
	C4FindObjectDef(C4PropList * def)
			: def(def) { }
private:
	C4PropList * def;
protected:
	bool Check(C4Object *pObj) override;
	bool IsImpossible() override;
};

class C4FindObjectInRect : public C4FindObject
{
public:
	C4FindObjectInRect(const C4Rect &rect)
			: rect(rect) { }
private:
	C4Rect rect;
protected:
	bool Check(C4Object *pObj) override;
	C4Rect *GetBounds() override { return &rect; }
	bool IsImpossible() override;
};

class C4FindObjectAtPoint : public C4FindObject
{
public:
	C4FindObjectAtPoint(int32_t x, int32_t y)
			: bounds(x, y, 1, 1) { }
private:
	C4Rect bounds;
protected:
	bool Check(C4Object *pObj) override;
	C4Rect *GetBounds() override { return &bounds; }
	bool UseShapes() override { return true; }
};

class C4FindObjectAtRect : public C4FindObject
{
public:
	C4FindObjectAtRect(int32_t x, int32_t y, int32_t wdt, int32_t hgt)
			: bounds(x, y, wdt, hgt) { }
private:
	C4Rect bounds;
protected:
	bool Check(C4Object *pObj) override;
	C4Rect *GetBounds() override { return &bounds; }
	bool UseShapes() override { return true; }
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
	bool Check(C4Object *pObj) override;
	C4Rect *GetBounds() override { return &bounds; }
	bool UseShapes() override { return true; }
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
	bool Check(C4Object *pObj) override;
	C4Rect *GetBounds() override { return &bounds; }
};

class C4FindObjectCone : public C4FindObject
{
public:
	C4FindObjectCone(int32_t x, int32_t y, int32_t r, int32_t angle, int32_t width, int32_t prec = 1)
			: x(x), y(y), r2(r * r), cone_angle(angle % (360 * prec)), cone_width(width), prec_angle(prec), bounds(x - r, y - r, 2 * r + 1, 2 * r + 1) { }
private:
	int32_t x, y, r2, cone_angle, cone_width, prec_angle;
	C4Rect bounds;
protected:
	bool Check(C4Object *pObj) override;
	C4Rect *GetBounds() override { return &bounds; }
};

class C4FindObjectOCF : public C4FindObject
{
public:
	C4FindObjectOCF(int32_t ocf)
			: ocf(ocf) { }
private:
	int32_t ocf;
protected:
	bool Check(C4Object *pObj) override;
	bool IsImpossible() override;
};

class C4FindObjectCategory : public C4FindObject
{
public:
	C4FindObjectCategory(int32_t iCategory)
			: iCategory(iCategory) { }
private:
	int32_t iCategory;
protected:
	bool Check(C4Object *pObj) override;
	bool IsEnsured() override;
};

class C4FindObjectAction : public C4FindObject
{
public:
	C4FindObjectAction(const char *szAction)
			: szAction(szAction) { }
private:
	const char *szAction;
protected:
	bool Check(C4Object *pObj) override;
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
	bool Check(C4Object *pObj) override;
};

class C4FindObjectProcedure : public C4FindObject
{
public:
	C4FindObjectProcedure(C4String * procedure)
			: procedure(procedure) { /* no need to incref, the pointer is never dereferenced */ }
private:
	C4String * procedure;
protected:
	bool Check(C4Object *pObj) override;
	bool IsImpossible() override;
};

class C4FindObjectContainer : public C4FindObject
{
public:
	C4FindObjectContainer(C4Object *pContainer)
			: pContainer(pContainer) { }
private:
	C4Object *pContainer;
protected:
	bool Check(C4Object *pObj) override;
};

class C4FindObjectAnyContainer : public C4FindObject
{
public:
	C4FindObjectAnyContainer() = default;
protected:
	bool Check(C4Object *pObj) override;
};

class C4FindObjectOwner : public C4FindObject
{
public:
	C4FindObjectOwner(int32_t iOwner)
			: iOwner(iOwner) { }
private:
	int32_t iOwner;
protected:
	bool Check(C4Object *pObj) override;
	bool IsImpossible() override;
};

class C4FindObjectController : public C4FindObject
{
public:
	C4FindObjectController(int32_t controller)
			: controller(controller) { }
private:
	int32_t controller;
protected:
	bool Check(C4Object *pObj) override;
	bool IsImpossible() override;
};

class C4FindObjectFunc : public C4FindObject
{
public:
	C4FindObjectFunc(C4String * Name): Name(Name) { }
	void SetPar(int i, const C4Value &val);
private:
	C4String * Name;
	C4AulParSet Pars;
protected:
	bool Check(C4Object *pObj) override;
	bool IsImpossible() override;
};

class C4FindObjectProperty : public C4FindObject
{
public:
	C4FindObjectProperty(C4String * Name) : Name(Name) { }
private:
	C4String * Name;
protected:
	bool Check(C4Object *pObj) override;
	bool IsImpossible() override;
};

class C4FindObjectLayer : public C4FindObject
{
public:
	C4FindObjectLayer(C4Object *pLayer) : pLayer(pLayer) {}
private:
	C4Object *pLayer;
protected:
	bool Check(C4Object *pObj) override;
	bool IsImpossible() override;
};

class C4FindObjectInArray : public C4FindObject
{
public:
	C4FindObjectInArray(C4ValueArray *pArray) : pArray(pArray) {}
private:
	C4ValueArray *pArray;
protected:
	bool Check(C4Object *pObj) override;
	bool IsImpossible() override;
};

// result sorting
class C4SortObject
{
public:
	C4SortObject() = default;
	virtual ~C4SortObject() = default;

public:
	// Overridables
	virtual int32_t Compare(C4Object *pObj1, C4Object *pObj2) = 0; // return value <0 if obj1 is to be sorted before obj2

	virtual bool PrepareCache(const C4ValueArray *pObjs) { return false; }
	virtual int32_t CompareCache(int32_t iObj1, int32_t iObj2, C4Object *pObj1, C4Object *pObj2) { return Compare(pObj1, pObj2); }

public:
	static C4SortObject *CreateByValue(const C4Value &Data, const C4Object *context=nullptr);
	static C4SortObject *CreateByValue(int32_t iType, const C4ValueArray &Data, const C4Object *context=nullptr);

	void SortObjects(C4ValueArray *pArray);
};

class C4SortObjectByValue : public C4SortObject
{
public:
	C4SortObjectByValue();
	~C4SortObjectByValue() override;

private:
	int32_t *pVals{nullptr};
	int32_t iSize{0};

public:
	// Overridables
	int32_t Compare(C4Object *pObj1, C4Object *pObj2) override;
	virtual int32_t CompareGetValue(C4Object *pOf) = 0;

	bool PrepareCache(const C4ValueArray *pObjs) override;
	int32_t CompareCache(int32_t iObj1, int32_t iObj2, C4Object *pObj1, C4Object *pObj2) override;

};

class C4SortObjectReverse : public C4SortObject // reverse sort
{
public:
	C4SortObjectReverse(C4SortObject *pSort)
			: C4SortObject(), pSort(pSort) {}
	~C4SortObjectReverse() override;
private:
	C4SortObject *pSort;

protected:
	int32_t Compare(C4Object *pObj1, C4Object *pObj2) override;

	bool PrepareCache(const C4ValueArray *pObjs) override;
	int32_t CompareCache(int32_t iObj1, int32_t iObj2, C4Object *pObj1, C4Object *pObj2) override;
};

class C4SortObjectMultiple : public C4SortObject // apply next sort if previous compares to equality
{
public:
	C4SortObjectMultiple(int32_t iCnt, C4SortObject **ppSorts, bool fFreeArray = true)
			: C4SortObject(), fFreeArray(fFreeArray), iCnt(iCnt), ppSorts(ppSorts) {}
	~C4SortObjectMultiple() override;
private:
	bool fFreeArray;
	int32_t iCnt;
	C4SortObject **ppSorts;

protected:
	int32_t Compare(C4Object *pObj1, C4Object *pObj2) override;

	bool PrepareCache(const C4ValueArray *pObjs) override;
	int32_t CompareCache(int32_t iObj1, int32_t iObj2, C4Object *pObj1, C4Object *pObj2) override;
};

class C4SortObjectDistance : public C4SortObjectByValue // sort by distance from point x/y
{
public:
	C4SortObjectDistance(int iX, int iY)
			: C4SortObjectByValue(), iX(iX), iY(iY) {}
private:
	int iX, iY;

protected:
	int32_t CompareGetValue(C4Object *pFor) override;
};

class C4SortObjectRandom : public C4SortObjectByValue // randomize order
{
public:
	C4SortObjectRandom() : C4SortObjectByValue() {}

protected:
	int32_t CompareGetValue(C4Object *pFor) override;
};

class C4SortObjectSpeed : public C4SortObjectByValue // sort by object xdir/ydir
{
public:
	C4SortObjectSpeed() : C4SortObjectByValue() {}

protected:
	int32_t CompareGetValue(C4Object *pFor) override;
};

class C4SortObjectMass : public C4SortObjectByValue // sort by mass
{
public:
	C4SortObjectMass() : C4SortObjectByValue() {}

protected:
	int32_t CompareGetValue(C4Object *pFor) override;
};

class C4SortObjectValue : public C4SortObjectByValue // sort by value
{
public:
	C4SortObjectValue() : C4SortObjectByValue() {}

protected:
	int32_t CompareGetValue(C4Object *pFor) override;
};

class C4SortObjectFunc : public C4SortObjectByValue // sort by script function
{
public:
	C4SortObjectFunc(C4String * Name): Name(Name) { }
	void SetPar(int i, const C4Value &val);
private:
	C4String * Name;
	C4AulParSet Pars;
protected:
	int32_t CompareGetValue(C4Object *pFor) override;
};

#endif
