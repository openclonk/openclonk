/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2012  Armin Burgmeier
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

#ifndef INC_C4Rope
#define INC_C4Rope

#include <stdexcept>
#include <C4Object.h>

// All units in pixels and frames

class C4RopeError: public std::runtime_error
{
public:
	C4RopeError(const std::string& message): std::runtime_error(message) {}
};

class C4Rope;
class C4RopeElement
{
	friend class C4Rope;
public:
	C4RopeElement(C4Object* obj, bool fixed);
	C4RopeElement(C4Real x, C4Real y, C4Real m, bool fixed);

	C4Real GetX() const { return Object ? GetTargetX() : x; }
	C4Real GetY() const { return Object ? GetTargetY() : y; }
	C4Real GetVx() const { return Object ? Object->xdir : vx; }
	C4Real GetVy() const { return Object ? Object->ydir : vy; }
	C4Real GetMass() const { return Object ? itofix(Object->Mass) : m; }
	C4Object* GetObject() const { return Object; }

	C4Real GetTargetX() const;
	C4Real GetTargetY() const;

	void AddForce(C4Real x, C4Real y);
	void Execute(const C4Rope* rope, C4Real dt);
private:
	void ResetForceRedirection(C4Real dt);
	void SetForceRedirection(const C4Rope* rope, int ox, int oy);
	bool SetForceRedirectionByLookAround(const C4Rope* rope, int ox, int oy, C4Real dx, C4Real dy, C4Real l, C4Real angle);

	bool Fixed; // Apply rope forces to this element?
	C4Real x, y; // pos
	C4Real vx, vy; // velocity
	C4Real m; // mass
	C4Real fx, fy; // force
	C4Real rx, ry; // force redirection
	C4Real rdt; // force redirection timeout
	C4Real fcx, fcy; // force after solve -- for debug output only
	C4RopeElement* Next; // next rope element, or NULL
	C4RopeElement* Prev; // prev rope element, or NULL
	C4Object* Object; // Connected object. If set, x/y/vx/vy/m are ignored.
	int LastContactVertex; // Vertex which most recently had collision with landscape
};

class C4Rope: public C4PropListNumbered
{
public:
	C4Rope(C4PropList* Prototype, C4Object* first_obj, C4Object* second_obj, C4Real segment_length, C4DefGraphics* graphics);
	~C4Rope();

	void Draw(C4TargetFacet& cgo, C4BltTransform* pTransform);
	void Execute();

	void ClearPointers(C4Object* obj);

	C4Real GetSegmentLength() const { return l; }
	C4Real GetOuterFriction() const { return mu; }

	C4RopeElement* GetFront() const { return Front; }
	C4RopeElement* GetBack() const { return Back; }
	void SetFront(C4Object* obj, C4Real x, C4Real y) { Front->Object = obj; Front->x = x; Front->y = y; Front->LastContactVertex = -1; }
	void SetBack(C4Object* obj, C4Real x, C4Real y) { Back->Object = obj; Back->x = x; Back->y = y; Back->LastContactVertex = -1; }

	C4Real GetFrontAutoSegmentation() const { return FrontAutoSegmentation; }
	C4Real GetBackAutoSegmentation() const { return BackAutoSegmentation; }
	void SetFrontAutoSegmentation(C4Real max) { FrontAutoSegmentation = max; }
	void SetBackAutoSegmentation(C4Real max) { BackAutoSegmentation = max; }

	bool GetFrontFixed() const { return Front->Fixed; }
	bool GetBackFixed() const { return Back->Fixed; }
	void SetFrontFixed(bool fixed) { Front->Fixed = fixed; }
	void SetBackFixed(bool fixed) { Back->Fixed = fixed; }

	void PullFront(C4Real f) { FrontPull = f; }
	void PullBack(C4Real f) { BackPull = f; }
private:
	C4Real GetL(const C4RopeElement* prev, const C4RopeElement* next) const;

	void DoAutoSegmentation(C4RopeElement* fixed, C4RopeElement* first, C4Real max);
	void Solve(C4RopeElement* prev, C4RopeElement* next);

	// Whether to apply repulsive forces between rope segments.
	// TODO: Could be made a property...
	static const bool ApplyRepulsive = false;

	unsigned int NumIterations; // Number of iterations per frame
	const float Width; // Width of rope
	C4DefGraphics* Graphics;
	int32_t SegmentCount;

	C4Real l; // spring length in equilibrium
	C4Real k; // spring constant
	C4Real mu; // outer friction constant
	C4Real eta; // inner friction constant

	C4RopeElement* Front;
	C4RopeElement* Back;

	C4Real FrontAutoSegmentation;
	C4Real BackAutoSegmentation;

	C4Real FrontPull;
	C4Real BackPull;
};

class C4RopeAul: public C4AulScript
{
public:
	C4RopeAul();
	virtual ~C4RopeAul();

	virtual bool Delete() { return false; }
	virtual C4PropListStatic* GetPropList() { return RopeDef; }

	void InitFunctionMap(C4AulScriptEngine* pEngine);

protected:
	C4PropListStatic* RopeDef;
};

class C4RopeList
{
public:
	C4RopeList();
	
	void InitFunctionMap(C4AulScriptEngine* pEngine) { RopeAul.InitFunctionMap(pEngine); }

	void Execute();
	void Draw(C4TargetFacet& cgo, C4BltTransform* pTransform);

	C4Rope* CreateRope(C4Object* first_obj, C4Object* second_obj, C4Real segment_length, C4DefGraphics* graphics);
	void RemoveRope(C4Rope* rope);

	void ClearPointers(C4Object* obj);

private:
	C4RopeAul RopeAul;
	std::vector<C4Rope*> Ropes;
};

#endif // INC_C4Rope
