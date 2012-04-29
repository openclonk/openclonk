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

class C4RopeSegment
{
	friend class C4Rope;
public:
	C4RopeSegment(C4Real x, C4Real y, C4Real m);

	C4Real GetX() const { return x; }
	C4Real GetY() const { return y; }
	C4Real GetVx() const { return vx; }
	C4Real GetVy() const { return vy; }
	C4Real GetMass() const { return m; }

	void AddForce(C4Real x, C4Real y);

	void Execute(C4Real dt, C4Real mu);
private:
	C4Real x, y; // pos
	C4Real vx, vy; // velocity
	C4Real fx, fy; // force
	C4Real m; // mass
	C4RopeSegment* next;
	C4RopeSegment* prev;
};

class C4RopeEnd
{
	friend class C4Rope;
public:
	C4RopeEnd(C4RopeSegment* segment, C4Object* obj, bool fixed);
	C4RopeEnd(C4RopeSegment* segment, C4Real x, C4Real y, C4Real m, bool fixed);

	C4Real GetX() const { return has_object ? obj->fix_x : end.x; }
	C4Real GetY() const { return has_object ? obj->fix_y : end.y; }
	C4Real GetVx() const { return has_object ? obj->xdir : end.vx; }
	C4Real GetVy() const { return has_object ? obj->ydir : end.vy; }
	C4Real GetMass() const { return has_object ? itofix(obj->Mass) : end.m; }
	C4Object* GetObject() const { return has_object ? obj : NULL; }

	void AddForce(C4Real fx, C4Real fy);
	void Execute(C4Real dt);

private:
	C4RopeSegment* segment;

	bool has_object;
	bool fixed;

	C4Real fx, fy;

	union
	{
		struct {
			C4Real x, y; // pos
			C4Real vx, vy; // velocity
			C4Real m; // mass
		} end;
		C4Object* obj;
	};
};

class C4Rope: public C4PropListNumbered
{
public:
	C4Rope(C4PropList* Prototype, C4Object* first_obj, C4Object* second_obj, int32_t n_segments, C4DefGraphics* graphics);
	~C4Rope();

	void Draw(C4TargetFacet& cgo, C4BltTransform* pTransform);

	void Execute();

	C4Object* GetFront() const { return front->GetObject(); }
	C4Object* GetBack() const { return back->GetObject(); }
private:
	template<typename TRopeType1, typename TRopeType2>
	void Solve(TRopeType1* prev, TRopeType2* next);

	const float w; // Width of rope
	C4DefGraphics* Graphics;

	int32_t n_segments;

	// TODO: Add a "dynlength" feature which adapts the spring length to the
	// distance of the two ends, up to a maximum... and/or callbacks to script
	// when the length should be changed so that script can do it (and maybe
	// play an animation, such as for the lift tower).

	C4Real l; // spring length in equilibrium
	C4Real k; // spring constant
	C4Real mu; // outer friction constant
	C4Real eta; // inner friction constant

	C4RopeEnd* front;
	C4RopeEnd* back;

	unsigned int n_iterations;
};

class C4RopeAul: public C4AulScript
{
public:
	C4RopeAul();
	virtual ~C4RopeAul();

	virtual bool Delete() { return false; }
	virtual C4PropList* GetPropList() { return RopeDef; }

	void InitFunctionMap(C4AulScriptEngine* pEngine);

protected:
	C4PropList* RopeDef;
};

class C4RopeList
{
public:
	C4RopeList();
	
	void InitFunctionMap(C4AulScriptEngine* pEngine) { RopeAul.InitFunctionMap(pEngine); }

	void Execute();
	void Draw(C4TargetFacet& cgo, C4BltTransform* pTransform);

	C4Rope* CreateRope(C4Object* first_obj, C4Object* second_obj, int32_t n_segments, C4DefGraphics* graphics);

private:
	C4RopeAul RopeAul;
	std::vector<C4Rope*> Ropes;
};

#endif // INC_C4Rope
