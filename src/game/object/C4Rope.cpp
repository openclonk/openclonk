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
	struct Vertex {
		Vertex() {}
		Vertex(float x, float y): x(x), y(y) {}

		float x;
		float y;
	};

	// For use in initializer list
	C4Real ObjectDistance(C4Object* first, C4Object* second)
	{
		C4Real dx = second->fix_x - first->fix_x;
		C4Real dy = second->fix_y - first->fix_y;
		return ftofix(sqrt(fixtof(dx*dx + dy*dy))); // TODO: Replace by integer sqrt
	}

	// To be used by the Solve() template function:
	C4Object* GetObject(C4RopeSegment* segment) { return NULL; }
	C4Object* GetObject(C4RopeEnd* end) { return end->GetObject(); }

	// Helper function for Draw: determines vertex positions for one segment
	void VertexPos(Vertex& out1, Vertex& out2, Vertex& out3, Vertex& out4,
		             const Vertex& v1, const Vertex& v2, float w)
	{
		const float l = sqrt( (v1.x - v2.x)*(v1.x - v2.x) + (v1.y - v2.y)*(v1.y - v2.y));

		out1.x = v1.x + w/2.0f * (v1.y - v2.y) / l;
		out1.y = v1.y - w/2.0f * (v1.x - v2.x) / l;
		out2.x = v1.x - w/2.0f * (v1.y - v2.y) / l;
		out2.y = v1.y + w/2.0f * (v1.x - v2.x) / l;
		out3.x = v2.x + w/2.0f * (v1.y - v2.y) / l;
		out3.y = v2.y - w/2.0f * (v1.x - v2.x) / l;
		out4.x = v2.x - w/2.0f * (v1.y - v2.y) / l;
		out4.y = v2.y + w/2.0f * (v1.x - v2.x) / l;
	}

	// Copied from StdGL.cpp... actually the rendering code should be moved
	// there so we don't need to duplicate it here.
	bool ApplyZoomAndTransform(float ZoomX, float ZoomY, float Zoom, C4BltTransform* pTransform)
	{
		// Apply zoom
		glTranslatef(ZoomX, ZoomY, 0.0f);
		glScalef(Zoom, Zoom, 1.0f);
		glTranslatef(-ZoomX, -ZoomY, 0.0f);

		// Apply transformation
		if (pTransform)
		{
			const GLfloat transform[16] = { pTransform->mat[0], pTransform->mat[3], 0, pTransform->mat[6], pTransform->mat[1], pTransform->mat[4], 0, pTransform->mat[7], 0, 0, 1, 0, pTransform->mat[2], pTransform->mat[5], 0, pTransform->mat[8] };
			glMultMatrixf(transform);

			// Compute parity of the transformation matrix - if parity is swapped then
			// we need to cull front faces instead of back faces.
			const float det = transform[0]*transform[5]*transform[15]
			                  + transform[4]*transform[13]*transform[3]
			                  + transform[12]*transform[1]*transform[7]
			                  - transform[0]*transform[13]*transform[7]
			                  - transform[4]*transform[1]*transform[15]
			                  - transform[12]*transform[5]*transform[3];
			return det > 0;
		}

		return true;
	}
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

				// TODO: Remove contact attachment if there is much force in opposite direction
				// TODO: V redirection
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
	w(5.0f), n_segments(n_segments), l(ObjectDistance(first_obj, second_obj) / (n_segments + 1)),
	k(Fix1*3), eta(Fix1*3), /* TODO: proper default values for k and eta */ n_iterations(20)
{
	if(!PathFree(first_obj->GetX(), first_obj->GetY(), second_obj->GetX(), second_obj->GetY()))
		throw C4RopeError("Path between objects is blocked");
	if(n_segments < 1)
		throw C4RopeError("Segments < 1 given");

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
	fx += (prev->GetVx() - next->GetVx()) * eta;
	fy += (prev->GetVy() - next->GetVy()) * eta;

	// Could add air/water friction here

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

// TODO: Move this to StdGL
void C4Rope::Draw(C4TargetFacet& cgo, C4BltTransform* pTransform)
{
	Vertex Tmp[4];
	Vertex Vertices[n_segments*2+4];

	VertexPos(Vertices[0], Vertices[1], Tmp[0], Tmp[1],
	          Vertex(fixtof(front->GetX()), fixtof(front->GetY())),
	          Vertex(fixtof(front->segment->GetX()), fixtof(front->segment->GetY())), w);

	unsigned int i = 2;
	bool parity = true;
	for(C4RopeSegment* cur = front->segment; cur != NULL; cur = cur->next, i += 2)
	{
		Vertex v1(fixtof(cur->GetX()),
		          fixtof(cur->GetY()));
		Vertex v2(fixtof(cur->next ? cur->next->GetX() : back->GetX()), 
		          fixtof(cur->next ? cur->next->GetY() : back->GetY()));
		Vertex v3(fixtof(cur->prev ? cur->prev->GetX() : front->GetX()),
		          fixtof(cur->prev ? cur->prev->GetY() : front->GetY()));

		// Parity -- parity swaps for each pointed angle (<90 deg)
		float cx = v1.x - v3.x;
		float cy = v1.y - v3.y;
		float ex = v1.x - v2.x;
		float ey = v1.y - v2.y;
		if(cx*ex+cy*ey > 0)
			parity = !parity;

		// Obtain vertex positions
		if(parity)
			VertexPos(Tmp[2], Tmp[3], Vertices[i+2], Vertices[i+3], v1, v2, w);
		else
			VertexPos(Tmp[3], Tmp[2], Vertices[i+3], Vertices[i+2], v1, v2, w);
		Tmp[2].x = (Tmp[0].x + Tmp[2].x)/2.0f;
		Tmp[2].y = (Tmp[0].y + Tmp[2].y)/2.0f;
		Tmp[3].x = (Tmp[1].x + Tmp[3].x)/2.0f;
		Tmp[3].y = (Tmp[1].y + Tmp[3].y)/2.0f;

		// renormalize
		float dx = Tmp[3].x - Tmp[2].x;
		float dy = Tmp[3].y - Tmp[2].y;
		float dx2 = Vertices[i-1].x - Vertices[i-2].x;
		float dy2 = Vertices[i-1].y - Vertices[i-2].y;
		const float d = (dx2*dx2+dy2*dy2)/(dx*dx2+dy*dy2);
		Vertices[i  ].x = ( (Tmp[2].x + Tmp[3].x)/2.0f) - BoundBy((Tmp[3].x - Tmp[2].x)*d, -w, w)/2.0f;
		Vertices[i  ].y = ( (Tmp[2].y + Tmp[3].y)/2.0f) - BoundBy((Tmp[3].y - Tmp[2].y)*d, -w, w)/2.0f;
		Vertices[i+1].x = ( (Tmp[2].x + Tmp[3].x)/2.0f) + BoundBy((Tmp[3].x - Tmp[2].x)*d, -w, w)/2.0f;
		Vertices[i+1].y = ( (Tmp[2].y + Tmp[3].y)/2.0f) + BoundBy((Tmp[3].y - Tmp[2].y)*d, -w, w)/2.0f;

		Tmp[0] = Vertices[i+2];
		Tmp[1] = Vertices[i+3];
	}

	glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
	glVertexPointer(2, GL_FLOAT, 0, Vertices);
	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);
	glDrawArrays(GL_QUAD_STRIP, 0, n_segments*2+4);
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

void C4RopeList::Draw(C4TargetFacet& cgo, C4BltTransform* pTransform)
{
	ZoomData z;
	pDraw->GetZoom(&z);

	glPushMatrix();
	ApplyZoomAndTransform(z.X, z.Y, z.Zoom, pTransform);
	glTranslatef(cgo.X-cgo.TargetX, cgo.Y-cgo.TargetY, 0.0f);

	for(unsigned int i = 0; i < Ropes.size(); ++i)
		Ropes[i]->Draw(cgo, pTransform);

	glPopMatrix();
}
