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

#include <C4Include.h>
#include <C4Landscape.h>
#include <C4Rope.h>

namespace
{
	C4Real ObjectDistance(C4Object* first, C4Object* second)
	{
		C4Real dx = second->fix_x - first->fix_x;
		C4Real dy = second->fix_y - first->fix_y;
		return ftofix(sqrt(fixtof(dx*dx + dy*dy))); // TODO: Replace by integer sqrt
	}

	C4Object* GetObject(C4RopeSegment* segment) { return NULL; }
	C4Object* GetObject(C4RopeEnd* end) { return end->GetObject(); }
}

C4RopeSegment::C4RopeSegment(C4Real x, C4Real y, C4Real m):
	x(x), y(y), vx(Fix0), vy(Fix0), fx(Fix0), fy(Fix0),
	m(m), next(NULL), prev(NULL)
{
}

void C4RopeSegment::AddForce(C4Real x, C4Real y)
{
	fx += x;
	fy += y;
}

void C4RopeSegment::Execute(C4Real dt)
{
	vx += dt * fx / m;
	vy += dt * fy / m;
	
	//if(vx*vx + vy*vy > 0)//itofix(1,10000))
	{
		int old_x = fixtoi(x);
		int old_y = fixtoi(y);
		int new_x = fixtoi(x + dt * vx);
		int new_y = fixtoi(y + dt * vy);
		int max_p = Max(abs(new_x - old_x), abs(new_y - old_y));

		int prev_x = old_x;
		int prev_y = old_y;
		bool hit = false;
		for(int i = 1; i <= max_p; ++i)
		{
			int inter_x = old_x + i * (new_x - old_x) / max_p;
			int inter_y = old_y + i * (new_y - old_y) / max_p;
			if(GBackSolid(inter_x, inter_y))
			{
				/*if(inter_x != old_x)*/ x = prev_x;
				/*if(inter_y != old_y)*/ y = prev_y;
				hit = true;

				// TODO: friction, v redirection
				vx = Fix0;
				vy = Fix0;

				break;
			}

			prev_x = inter_x;
			prev_y = inter_y;
		}

		if(!hit)
		{
			x += dt * vx;
			y += dt * vy;
		}
	}

	fx = fy = Fix0;
}

C4RopeEnd::C4RopeEnd(C4RopeSegment* segment, C4Object* obj, bool fixed):
	segment(segment), has_object(true), fixed(fixed), fx(Fix0), fy(Fix0)
{
	this->obj = obj;
}

C4RopeEnd::C4RopeEnd(C4RopeSegment* segment, C4Real x, C4Real y, C4Real m, bool fixed):
	segment(segment), has_object(false), fixed(fixed), fx(Fix0), fy(Fix0)
{
	this->end.x = x;
	this->end.y = y;
	this->end.vx = Fix0;
	this->end.vy = Fix0;
	this->end.m = m;
}

void C4RopeEnd::AddForce(C4Real x, C4Real y)
{
	fx += x;
	fy += y;
}

void C4RopeEnd::Execute(C4Real dt)
{
	if(!fixed)
	{
		if(has_object)
		{
			// StaticBacks don't move, so don't apply xdir/ydir to them
			// or the rope will behave weird
			if( (obj->Category & C4D_StaticBack) == 0)
			{
				obj->xdir += dt * fx / obj->Mass;
				obj->ydir += dt * fy / obj->Mass;
			}
		}
		else
		{
			// TODO: Share code for landscape collision with C4RopeSegment
			end.vx += dt * fx / end.m;
			end.vy += dt * fy / end.m;

			end.x += dt * end.vx;
			end.y += dt * end.vy;
		}
	}

	fx = fy = Fix0;
}

C4Rope::C4Rope(C4Object* first_obj, C4Object* second_obj, int32_t n_segments):
	n_segments(n_segments), l(ObjectDistance(first_obj, second_obj) / (n_segments + 1)),
	k(Fix1*3), rho(Fix1*3), /* TODO: proper default values for k and rho */ n_iterations(20)
{
	if(!PathFree(first_obj->GetX(), first_obj->GetY(), second_obj->GetX(), second_obj->GetY()))
		throw C4RopeError("Failed to create rope: Path between objects is blocked");
	if(n_segments < 1)
		throw C4RopeError("Failed to create rope: Segments < 1 given");

	// TODO: Have this as an array, not as a linked list -- it's ~static after all!
	const C4Real m(Fix1);
	C4RopeSegment* first_seg = NULL;
	C4RopeSegment* prev_seg = NULL;
	for(int32_t i = 0; i < n_segments; ++i)
	{
		C4RopeSegment* seg = new C4RopeSegment(first_obj->fix_x + (second_obj->fix_x - first_obj->fix_x) * (i+1) / (n_segments+1),
		                                       first_obj->fix_y + (second_obj->fix_y - first_obj->fix_y) * (i+1) / (n_segments+1),
		                                       /*l * ,*/ m);
		seg->prev = prev_seg;
		if(!prev_seg) first_seg = seg;
		else prev_seg->next = seg;

		prev_seg = seg;
	}

	front = new C4RopeEnd(first_seg, first_obj, true);
	back = new C4RopeEnd(prev_seg, second_obj, false);
}

C4Rope::~C4Rope()
{
	for(C4RopeSegment* cur = front->segment, *next; cur != NULL; cur = next)
	{
		next = cur->next;
		delete cur;
	}

	delete front;
	delete back;
}

template<typename TRopeType1, typename TRopeType2>
void C4Rope::Solve(TRopeType1* prev, TRopeType2* next) //C4RopeSegment* prev, C4RopeSegment* next)
{
	C4Real fx = Fix0, fy = Fix0;

	// Rope forces
	C4Real dx = prev->GetX() - next->GetX();
	C4Real dy = prev->GetY() - next->GetY();
	// TODO: Avoid floating point here by using an integer-based Sqrt algorithm
	// TODO: Could also use an approximation which works without Sqrt, especially
	// if this becomes a performance bottleneck. http://www.azillionmonkeys.com/qed/sqroot.html
	C4Real d = ftofix(sqrt(fixtof(dx*dx + dy*dy))); 
	if(d != 0)
	{
		fx += (dx / d) * k * (d - l);
		fy += (dy / d) * k * (d - l);
	}

	// Inner friction
	fx += (prev->GetVx() - next->GetVx()) * rho;
	fy += (prev->GetVy() - next->GetVy()) * rho;

	// Apply forces to masses. Don't apply gravity to objects since it's applied already in C4Object execution.
	prev->AddForce(-fx, -fy + (GetObject(prev) ? Fix0 : prev->GetMass() * ::Landscape.Gravity/5));
	next->AddForce(fx, fy + (GetObject(next) ? Fix0 : next->GetMass() * ::Landscape.Gravity/5));
}

void C4Rope::Execute()
{
	C4Real dt = itofix(1, n_iterations);
	for(unsigned int i = 0; i < n_iterations; ++i)
	{
		Solve(front, front->segment);
		for(C4RopeSegment* cur = front->segment; cur != NULL; cur = cur->next)
		{
			if(cur->next)
				Solve(cur, cur->next);
			else
				 Solve(cur, back);
		}

		for(C4RopeSegment* cur = front->segment; cur != NULL; cur = cur->next)
			cur->Execute(dt);
		front->Execute(dt);
		back->Execute(dt);
	}
}

void C4Rope::Draw(C4Facet& cgo)
{
	struct Vertex
	{
		float x, y;
	};

	// TODO: Change this so that it draws a quad strip
	Vertex Vertices[n_segments*4+4]; unsigned int i = 0;
	for(C4RopeSegment* cur = front->segment; cur != NULL; cur = cur->next)
	{
		float prev_x = fixtof(cur->prev ? cur->prev->GetX() : front->GetX());
		float prev_y = fixtof(cur->prev ? cur->prev->GetY() : front->GetY());
		float x = fixtof(cur->GetX());
		float y = fixtof(cur->GetY());

		float l = sqrt( (prev_x - x) * (prev_x - x) + (prev_y - y) * (prev_y - y));

		float x1 = prev_x + 2.5 * (prev_y - y) / l;
		float x2 = prev_x - 2.5 * (prev_y - y) / l;
		float x3 = x - 2.5 * (prev_y - y) / l;
		float x4 = x + 2.5 * (prev_y - y) / l;

		float y1 = prev_y - 2.5 * (prev_x - x) / l;
		float y2 = prev_y + 2.5 * (prev_x - x) / l;
		float y3 = y + 2.5 * (prev_x - x) / l;
		float y4 = y - 2.5 * (prev_x - x) / l;

		Vertices[i].x = x1;
		Vertices[i].y = y1;
		Vertices[i+1].x = x2;
		Vertices[i+1].y = y2;
		Vertices[i+2].x = x3;
		Vertices[i+2].y = y3;
		Vertices[i+3].x = x4;
		Vertices[i+3].y = y4;
		i += 4;
	}

	glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
	glVertexPointer(2, GL_FLOAT, 0, Vertices);
	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);
	glDrawArrays(GL_QUADS, 0, n_segments*4);
}

C4RopeList::C4RopeList()
{
	for(unsigned int i = 0; i < Ropes.size(); ++i)
		delete Ropes[i];
}

C4Rope* C4RopeList::CreateRope(C4Object* first_obj, C4Object* second_obj, int32_t n_segments)
{
	Ropes.push_back(new C4Rope(first_obj, second_obj, n_segments));
	return Ropes.back();
}

void C4RopeList::Execute()
{
	for(unsigned int i = 0; i < Ropes.size(); ++i)
		Ropes[i]->Execute();
}

void C4RopeList::Draw(C4Facet& cgo)
{
	for(unsigned int i = 0; i < Ropes.size(); ++i)
		Ropes[i]->Draw(cgo);
}
