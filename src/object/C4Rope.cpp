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

#define C4ROPE_DRAW_DEBUG

namespace
{
	struct Vertex {
		Vertex() {}
		Vertex(float x, float y): x(x), y(y) {}

		float x;
		float y;
	};

	struct DrawVertex: Vertex {
		float u;
		float v;
	};

	// TODO: If the sqrts become a performance bottleneck we could also use an
	// approximation which works without Sqrt, cf. http://www.azillionmonkeys.com/qed/sqroot.html
	C4Real Len(C4Real dx, C4Real dy)
	{
		// Prevent possible overflow
		if(Abs(dx) > 120 || Abs(dy) > 120)
			return itofix(SqrtI(fixtoi(dx)*fixtoi(dx) + fixtoi(dy)*fixtoi(dy)));// ftofix(sqrt(fixtoi(dx)*fixtoi(dx) + fixtoi(dy)*fixtoi(dy)));
		else
			return Sqrt(dx*dx + dy*dy);//ftofix(sqrt(fixtof(dx*dx + dy*dy)));
	}

	// For use in initializer list
	C4Real ObjectDistance(C4Object* first, C4Object* second)
	{
		C4Real dx = second->fix_x - first->fix_x;
		C4Real dy = second->fix_y - first->fix_y;
		return Len(dx, dy);
	}

	// Helper function for Draw: determines vertex positions for one segment
	void VertexPos(Vertex& out1, Vertex& out2, Vertex& out3, Vertex& out4,
		             const Vertex& v1, const Vertex& v2, float w)
	{
		// This is for graphics only, so plain sqrt() is OK.
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

	// PathFinder callback function. This saves the first and last waypoint which
	// determine the direction toward which to pull.
	struct PullPathInfo
	{
		bool FirstSeg;
		const int32_t bx, by;
		const int32_t ex, ey;
		int32_t px, py;
		int32_t nx, ny;
		C4Real d;
	};

	bool PullPathAccumulator(int32_t iX, int32_t iY, intptr_t iTransferTarget, intptr_t PathInfoPtr)
	{
		PullPathInfo* Info = (PullPathInfo*)PathInfoPtr;

		// Ignore points which are very close (~7 pixels) to start or end. Such
		// points are often misleading because the PathFinder first pulls away a
		// bit from the ground before actually moving toward the destination.
		if(Info->px != iX || Info->py != iY)
		{
			if(Info->FirstSeg)
			{
				if( (Info->ex - iX) * (Info->ex - iX) + (Info->ey - iY) * (Info->ey - iY) > 50)
				{
					Info->nx = iX;
					Info->ny = iY;
					Info->FirstSeg = false;
				}
			}

			if( (Info->bx - iX) * (Info->bx - iX) + (Info->by - iY) * (Info->by - iY) > 50)
			{
				C4Real dx = itofix(Info->px - iX);
				C4Real dy = itofix(Info->py - iY);
				Info->px = iX;
				Info->py = iY;
				Info->d += Len(dx, dy);
			}
		}

		return true; // note return value is ignored
	}
}

C4RopeElement::C4RopeElement(C4Object* obj, bool fixed):
	Fixed(fixed), oldx(obj->fix_x), oldy(obj->fix_y), fx(Fix0), fy(Fix0),
	rx(Fix0), ry(Fix0), rdt(Fix0), fcx(Fix0), fcy(Fix0),
	Next(NULL), Prev(NULL), FirstLink(NULL), LastLink(NULL),
	Object(obj), LastContactVertex(-1)
{
}

C4RopeElement::C4RopeElement(C4Real x, C4Real y, C4Real m, bool fixed):
	Fixed(fixed), x(x), y(y), oldx(x), oldy(y), vx(Fix0), vy(Fix0), m(m),
	fx(Fix0), fy(Fix0), rx(Fix0), ry(Fix0), rdt(Fix0), fcx(Fix0), fcy(Fix0),
	Next(NULL), Prev(NULL), FirstLink(NULL), LastLink(NULL),
	Object(NULL), LastContactVertex(-1)
{
}

C4RopeElement::~C4RopeElement()
{
	for(C4RopeLink* link = FirstLink, *next; link != NULL; link = next)
	{
		next = link->Next;
		delete link;
	}
}

void C4RopeElement::AddForce(C4Real x, C4Real y)
{
	fx += x;
	fy += y;

#ifdef C4ROPE_DRAW_DEBUG
	fcx += x;
	fcy += y;
#endif
}

C4Real C4RopeElement::GetTargetX() const
{
	// TODO: Prevent against object changes: Reset when Target changes wrt
	// to the object LastContactVertex refers to.
	if(LastContactVertex == -1) return Object->fix_x;

	C4Object* obj = Object;
	while(obj->Contained)
		obj = obj->Contained;
	return obj->fix_x + itofix(obj->Shape.VtxX[LastContactVertex]);
}

C4Real C4RopeElement::GetTargetY() const
{
	// TODO: Prevent against object changes: Reset when Target changes wrt
	// to the object LastContactVertex refers to.
	if(LastContactVertex == -1) return Object->fix_y;

	C4Object* obj = Object;
	while(obj->Contained)
		obj = obj->Contained;
	return obj->fix_y + itofix(obj->Shape.VtxY[LastContactVertex]);
}

bool C4RopeElement::InsertLinkPosition(int from_x, int from_y, int to_x, int to_y, int link_x, int link_y, int& insert_x, int& insert_y)
{
	if(PathFree(to_x, to_y, link_x, link_y)) return false;

	// Sanity check, these might be violated if the landscape changed.
	// In that case the rope might end up buried in earth, in which case we
	// do not want to do any maneuvering around, but the rope is just stuck.
	if(!PathFree(from_x, from_y, link_x, link_y)) return false;

	// If the element has been moved through solid material, only take the part
	// of the way into account that is free.
	int stop_x, stop_y;
	if(!PathFree(from_x, from_y, to_x, to_y, &stop_x, &stop_y))
	{
		to_x = stop_x;
		to_y = stop_y;
	}

	// Find the position between from_x,from_y, and to_x,to_y
	// where there is no path free to link_x,link_y anymore.
	int prev_x = from_x;
	int prev_y = from_y;
	int coll_x = -1;
	int coll_y = -1;
	int max_p = Max(abs(to_x - from_x), abs(to_y - from_y));
	for(int i = 1; i <= max_p; ++i)
	{
		int inter_x = from_x + i * (to_x - from_x) / max_p;
		int inter_y = from_y + i * (to_y - from_y) / max_p;
		if(!PathFree(inter_x, inter_y, link_x, link_y))
		{
			coll_x = inter_x;
			coll_y = inter_y;
			break;
		}

		prev_x = inter_x;
		prev_y = inter_y;
	}

	// Such a position must have been found, because of the initial
	// PathFree(to_x, to_y, link_x, link_y) check.
	assert(coll_x != -1 && coll_y != -1);

	// Now we have:
	// prev_x,prev_y: Last position between from_x,from_y and to_x,to_y
	// where the path to link_x,link_y is still free.
	// coll_x,coll_y: First position between from_x,from_y and to_x,to_y
	// where the path to link_x,link_y is no longer free.
	// prev_x,prev_y and coll_x,coll_y are 1 pixel apart from each other.

	// Now the idea is to insert a new link somewhere on the line from
	// prev_x,prev_y to link_x,link_y such that
	// PathFree(coll_x, coll_y, insert_x, insert_y) holds and the point is
	// as close to link_x, link_y as possible.
	max_p = Max(abs(link_x - prev_x), abs(link_y - prev_y));
	for(int i = 1; i <= max_p; ++i)
	{
		int inter_x = link_x + i * (prev_x - link_x) / max_p;
		int inter_y = link_y + i * (prev_y - link_y) / max_p;

		// Again a sanity check, to account for pixel rounding mismatches
		if(!PathFree(inter_x, inter_y, link_x, link_y)) continue;

		if(PathFree(coll_x, coll_y, inter_x, inter_y))
		{
			insert_x = inter_x;
			insert_y = inter_y;
			return true;
		}
	}

	// The final point in the loop is prev_x, prev_y, and there should be a
	// free path from it, as per the sanity check precondition.
	assert(false);
	return false;
}

void C4RopeElement::InsertLink(C4RopeElement* from, C4RopeElement* to, int insert_x, int insert_y)
{
	assert(this == from || this == to);

	C4RopeLink* Link = new C4RopeLink;
	Link->x = itofix(insert_x);
	Link->y = itofix(insert_y);

	if(this == from)
	{
		assert(from->Next == to);

		Link->Next = from->FirstLink;
		from->FirstLink = Link;
		Link->Prev = NULL;

		if(to->LastLink == NULL)
			to->LastLink = Link;
	}
	else
	{
		assert(to->Prev == from);

		Link->Prev = to->LastLink;
		to->LastLink = Link;
		Link->Next = NULL;

		if(from->FirstLink == NULL)
			from->FirstLink = Link;
	}
}

void C4RopeElement::Execute(const C4Rope* rope, C4Real dt)
{
	ResetForceRedirection(dt);

	// If attached object is contained, apply force to container
	C4Object* Target = Object;
	if(Target)
		while(Target->Contained)
			Target = Target->Contained;

	// Apply sticking friction
	if(!Target)
	{
		// Sticking friction: If a segment has contact with the landscape and it
		// is at rest then one needs to exceed a certain threshold force until it
		// starts moving.
		int ix = fixtoi(x);
		int iy = fixtoi(y);
		if(GBackSolid(ix+1, iy) || GBackSolid(ix-1, iy) || GBackSolid(ix, iy-1) || GBackSolid(ix, iy+1))
		{
			if(vx*vx + vy*vy < Fix1/4) // TODO: Threshold should be made a property
			{
				if(fx*fx + fy*fy < Fix1/4) // TODO: Threshold should be made a property
				{
					fx = fy = Fix0;
					vx = vy = Fix0;
					return;
				}
			}
		}
	}

	// Apply forces
	if(!Fixed)
	{
		if(!Target)
		{
			vx += dt * fx / m;
			vy += dt * fy / m;
		}
		else if( (Target->Category & C4D_StaticBack) == 0)
		{
			// Only apply xdir/ydir to targets if they are not attached
			if((Target->Action.t_attach & (CNAT_Left | CNAT_Right)) == 0)
				Target->xdir += dt * fx / Target->Mass;
			if((Target->Action.t_attach & (CNAT_Top | CNAT_Bottom)) == 0)
				Target->ydir += dt * fy / Target->Mass;
		}
	}
	fx = fy = Fix0;

	// Execute movement
	if(!Target)
	{
		// Compute old and new coordinates
		int old_x = fixtoi(x);
		int old_y = fixtoi(y);
		int new_x = fixtoi(x + dt * vx);
		int new_y = fixtoi(y + dt * vy);
		// Maximum distance in pixels in either X or Y
		int max_p = Max(abs(new_x - old_x), abs(new_y - old_y));

		// Check all pixels between old and new position
		int prev_x = old_x;
		int prev_y = old_y;
		bool hit = false;
		for(int i = 1; i <= max_p; ++i)
		{
			// Intermediate pixel position
			int inter_x = old_x + i * (new_x - old_x) / max_p;
			int inter_y = old_y + i * (new_y - old_y) / max_p;

			// Collision check
			if(GBackSolid(inter_x, inter_y))
			{
				x = itofix(prev_x);
				y = itofix(prev_y);
				hit = true;

				// Apply friction force
				fx -= rope->GetOuterFriction() * vx; fy -= rope->GetOuterFriction() * vy; 

				// Force redirection so that not every single pixel on a
				// chunky landscape is an obstacle for the rope.
				SetForceRedirection(rope, 0, 0);
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
	else
	{
		if(!Fixed)
		{
			// Object Force redirection if object has no contact attachment (if it
			// has then the procedure takes care of moving the object around
			// O(pixel) obstacles in the landscape).
			if(!Target->Action.t_attach && Target->xdir*Target->xdir + Target->ydir*Target->ydir >= Fix1)
			{
				// Check if the object has contact to the landscape
				//long iResult = 0;
				const DWORD dwCNATCheck = CNAT_Left | CNAT_Right | CNAT_Top | CNAT_Bottom;
				int iContactVertex = -1;
				for (int i = 0; i < Target->Shape.VtxNum; ++i)
					if(Target->Shape.GetVertexContact(i, dwCNATCheck, Target->GetX(), Target->GetY()))
						iContactVertex = i;

				if(iContactVertex != -1)
				{
					LastContactVertex = iContactVertex;
					SetForceRedirection(rope, Target->Shape.VtxX[iContactVertex], Target->Shape.VtxY[iContactVertex]);
				}
			}
		}
	}
}

void C4RopeElement::ResetForceRedirection(C4Real dt)
{
	// countdown+reset force redirection
	if(rdt != Fix0)
	{
		if(dt > rdt)
		{
			rx = ry = Fix0;
			rdt = Fix0;
		}
		else
		{
			rdt -= dt;
		}
	}
}

void C4RopeElement::SetForceRedirection(const C4Rope* rope, int ox, int oy)
{
	SetForceRedirectionByLookAround(rope, ox, oy, GetVx(), GetVy(), itofix(5), itofix(75));

#if 0
	if(!SetForceRedirectionByLookAround(rope, ox, oy, GetVx(), GetVy(), itofix(5), itofix(15)))
		if(!SetForceRedirectionByLookAround(rope, ox, oy, GetVx(), GetVy(), itofix(5), itofix(30)))
			SetForceRedirectionByLookAround(rope, ox, oy, GetVx(), GetVy(), itofix(5), itofix(45));
			//SetForceRedirectionByLookAround(rope, ox, oy, GetVx(), GetVy(), itofix(5), itofix(60));
#endif
}

bool C4RopeElement::SetForceRedirectionByLookAround(const C4Rope* rope, int ox, int oy, C4Real dx, C4Real dy, C4Real l, C4Real angle)
{
	// The procedure is the following: we try manuevering around
	// the obstacle, either left or right.  Which way we take is determined by
	// checking whether or not there is solid material in a 75 degree angle
	// relative to the direction of movement.
	const C4Real Cos75 = Cos(angle);
	const C4Real Sin75 = Sin(angle);

	C4Real vx1 =  Cos75 * dx + Sin75 * dy;
	C4Real vy1 = -Sin75 * dx + Cos75 * dy;
	C4Real vx2 =  Cos75 * dx - Sin75 * dy;
	C4Real vy2 =  Sin75 * dx + Cos75 * dy;
	const C4Real v = Len(dx, dy);

	// TODO: We should check more than a single pixel. There's some more potential for optimization here.
	if(v != Fix0 && !GBackSolid(ox + fixtoi(GetX() + vx1*l/v), oy + fixtoi(GetY() + vy1*l/v)))
	//if(v != Fix0 && PathFree(ox + fixtoi(GetX()), oy + fixtoi(GetY()), ox + fixtoi(GetX() + vx1*l/v), oy + fixtoi(GetY() + vy1*l/v)))
		{ rx = vx1/v; ry = vy1/v; rdt = Fix1/4; } // Enable force redirection for 1/4th of a frame
	else if(v != Fix0 && !GBackSolid(ox + fixtoi(GetX() + vx2/v), oy + fixtoi(GetY() + vy2/v)))
	//else if(v != Fix0 && PathFree(ox + fixtoi(GetX()), oy + fixtoi(GetY()), ox + fixtoi(GetX() + vx2/v), oy + fixtoi(GetY() + vy2/v)))
		{ rx = vx2/v; ry = vy2/v; rdt = Fix1/4; } // Enable force redirection for 1/4th of a frame
	else
		return false;
	return true;
}

C4Rope::C4Rope(C4PropList* Prototype, C4Object* first_obj, C4Object* second_obj, C4Real segment_length, C4DefGraphics* graphics):
	C4PropListNumbered(Prototype), Width(5.0f), Graphics(graphics), SegmentCount(fixtoi(ObjectDistance(first_obj, second_obj)/segment_length)),
	l(segment_length), k(Fix1*3), mu(Fix1*3), eta(Fix1*3), NumIterations(10),
	FrontAutoSegmentation(Fix0), BackAutoSegmentation(Fix0), FrontPull(Fix0), BackPull(Fix0)
{
	if(!PathFree(first_obj->GetX(), first_obj->GetY(), second_obj->GetX(), second_obj->GetY()))
		throw C4RopeError("Path between objects is blocked");
	if(Graphics->Type != C4DefGraphics::TYPE_Bitmap)
		throw C4RopeError("Can only use bitmap as rope graphics");

	Front = new C4RopeElement(first_obj, false);
	Back = new C4RopeElement(second_obj, false);

	const C4Real m(Fix1); // TODO: This should be a property

	C4RopeElement* prev_seg = Front;
	for(int32_t i = 0; i < SegmentCount; ++i)
	{
		// Create new element
		C4Real seg_x = first_obj->fix_x + (second_obj->fix_x - first_obj->fix_x) * (i+1) / (SegmentCount+1);
		C4Real seg_y = first_obj->fix_y + (second_obj->fix_y - first_obj->fix_y) * (i+1) / (SegmentCount+1);
		C4RopeElement* seg = new C4RopeElement(seg_x, seg_y, m, false);

		// Link it
		seg->Prev = prev_seg;
		prev_seg->Next = seg;
		prev_seg = seg;
	}

	// Link back segment
	prev_seg->Next = Back;
	Back->Prev = prev_seg;
}

C4Rope::~C4Rope()
{
	for(C4RopeElement* cur = Front, *next; cur != NULL; cur = next)
	{
		next = cur->Next;
		delete cur;
	}
}

C4Real C4Rope::GetL(const C4RopeElement* prev, const C4RopeElement* next) const
{
	// Normally the segment length is fixed at l, however if auto segmentation
	// is enabled then the first or last segments can be shorter.
	const C4Real dx = next->GetX() - prev->GetX();
	const C4Real dy = next->GetY() - prev->GetY();

	if(FrontAutoSegmentation > Fix0)
		if(prev == Front || next == Front)
			return Min(itofix(5), Len(dx, dy));
	if(BackAutoSegmentation > Fix0)
		if(prev == Back || next == Back)
			return Min(itofix(5), Len(dx, dy));

	return l;
}

void C4Rope::DoAutoSegmentation(C4RopeElement* fixed, C4RopeElement* first, C4Real max)
{
	// TODO: Should add a timeout to prevent oscillations: After one segment was
	// inserted do not allow segments to be removed in the same frame or couple
	// of frames, and vice versa. This gives the system a chance to get into
	// equilibrium before continuing with auto-segmentation.

	// Auto segmentation enabled?
	if(max > Fix0)
	{
		const C4Real dx = first->GetX() - fixed->GetX();
		const C4Real dy = first->GetY() - fixed->GetY();
		const C4Real lf = dx*dx+dy*dy;

		if(lf > l*l*itofix(15,10)*itofix(15,10) && l * SegmentCount < max)
		{
			const C4Real x = fixed->GetX() + itofix(5,10)*dx;
			const C4Real y = fixed->GetY() + itofix(5,10)*dy;
			C4RopeElement* new_elem = new C4RopeElement(x, y, first->GetMass(), false);
			new_elem->vx = (fixed->GetVx() + first->GetVx())/itofix(2);
			new_elem->vy = (fixed->GetVy() + first->GetVy())/itofix(2);

			// Link the new element
			if(fixed->Next == first)
			{
				new_elem->Prev = fixed;
				new_elem->Next = first;
				fixed->Next = new_elem;
				first->Prev = new_elem;
			}
			else
			{
				new_elem->Prev = first;
				new_elem->Next = fixed;
				fixed->Prev = new_elem;
				first->Next = new_elem;
			}
			++SegmentCount;
		}
		else if(SegmentCount > 0) // Rope cannot be shorter than just Beginning and End segment
		{
			// To find out whether we can shorten the rope we do the following:
			// We go through all elements and if at some point the nominal rope length
			// is shorter than the distance between that element and the fixpoint
			// and the path between the two is free, then the rope is shortened.
			unsigned int i = 1;
			for(C4RopeElement* cur = first; cur != NULL; cur = (fixed->Next == first ? cur->Next : cur->Prev), ++i)
			{
				// We use integers, not reals here, to protect for overflows. This works
				// because these numbers are large enough so we don't need to take care
				// about subpixel precision.
				const unsigned int nd = fixtoi(l*itofix(i));

				const unsigned int dx = fixtoi(cur->GetX() - fixed->GetX());
				const unsigned int dy = fixtoi(cur->GetY() - fixed->GetY());
				const unsigned int d2 = dx*dx+dy*dy;

				if(d2 > nd*nd*15/10*15/10)
					break;
				else if(d2 < nd*nd*8/10*8/10)
				{
					// TODO: Check whether all elements have PathFree, and stop if one hasn't?
					if(PathFree(fixtoi(fixed->GetX()), fixtoi(fixed->GetY()), fixtoi(cur->GetX()), fixtoi(cur->GetY())))
					{
						C4RopeElement* second = ((fixed->Next == first) ? first->Next : first->Prev);
						assert(second != NULL);

						// Remove first, relink fixed and second
						C4RopeElement* Del = first;

						if(fixed->Next == first)
						{
							fixed->Next = second;
							second->Prev = fixed;
						}
						else
						{
							fixed->Prev = second;
							second->Next = fixed;
						}

						delete Del;
						--SegmentCount;
					}

					break;
				}
			}
		}
	}
}

void C4Rope::Solve(C4RopeElement* prev, C4RopeElement* next)
{
	// Rope forces
	const C4Real dx = prev->GetX() - next->GetX();
	const C4Real dy = prev->GetY() - next->GetY();

	if(dx != Fix0 || dy != Fix0) //dx*dx + dy*dy > Fix0)
	{
		// Get segment length between prev and next
		const C4Real l = GetL(prev, next);

		// Compute forces between these points. If there is no material between the
		// rope segments, this is just a straight line. Otherwise, run the
		// pathfinder to find out in which direction to pull.
		// If the PathFinder doesn't find a path... then well.. no idea. Maybe
		// the rope got buried by an earthquake, or someone built a loam bridge
		// over a rope segment. In that case just apply the normal "straight"
		// forces and hope the best...

		// TODO: Avoid any of these expensive computations if both prev and next
		// have force redirection active

		int pix = fixtoi(prev->GetX());
		int piy = fixtoi(prev->GetY());
		int nix = fixtoi(next->GetX());
		int niy = fixtoi(next->GetY());

		// We only run the PathFinder when the distance between the two segments
		// is at least twice the nominal distance. 

		C4Real dx1, dy1, dx2, dy2, d;
		PullPathInfo Info = { true, pix, piy, nix, niy, nix, niy, pix, piy, Fix0 };
		if((Abs(dx) > l*2 || Abs(dy) > l*2 || dx*dx+dy*dy > l*l*4) && // The first two checks exist to avoid an overflow in the dx*dx+dy*dy expression
		   !PathFree(pix, piy, nix, niy) &&
		   Game.PathFinder.Find(pix, piy, nix, niy, PullPathAccumulator, (intptr_t)&Info))
		{
			C4Real dpx = itofix(Info.px - pix);
			C4Real dpy = itofix(Info.py - piy);
			C4Real dp = Len(dpx, dpy); // TODO: Could be computed in accumulator

			C4Real dnx = itofix(Info.nx - nix);
			C4Real dny = itofix(Info.ny - niy);
			C4Real dn = Len(dnx, dny); // TODO: Could be computed in accumulator

			if(dp != Fix0)
				{ dx1 = dpx / dp; dy1 = dpy / dp; }
			else
				{ dx1 = Fix0; dy1 = Fix0; }

			if(dn != Fix0)
				{ dx2 = dnx / dn; dy2 = dny / dn; }
			else
				{ dx2 = Fix0; dy2 = Fix0; }

			d = Info.d + dp;

			/*int index = 0;
			C4RopeElement* prev2 = prev;
			while(prev2 != Front) { prev2 = prev2->Prev; ++index; }

			printf("Solved %p(index=%d) via PathFinder, from %d/%d to %d/%d\n", this, index, pix, piy, nix, niy);
			printf(" --> p to %d/%d, n to %d/%d\n", Info.px, Info.py, Info.nx, Info.ny);
			printf(" --> vec_p=%f/%f, vec_n=%f/%f, d(pathfinder)=%f, d(direct)=%f\n", fixtof(dx1), fixtof(dy1), fixtof(dx2), fixtof(dy2), fixtof(d), fixtof(Len(dx,dy)));*/
		}
		else
		{
			d = Len(dx, dy);
			if(d != Fix0)
				{ dx1 = -(dx / d); dy1 = -(dy / d); }
			else
				{ dx1 = 0; dx2 = 0; }

			dx2 = -dx1;
			dy2 = -dy1;
		}

		if(ApplyRepulsive || d > l)
		{
			if(prev->rdt != Fix0) { dx1 = prev->rx; dy1 = prev->ry; }
			if(next->rdt != Fix0) { dx2 = next->rx; dy2 = next->ry; }

			prev->AddForce(dx1 * k * (d - l), dy1 * k * (d - l));
			next->AddForce(dx2 * k * (d - l), dy2 * k * (d - l));
		}
	}

	// Inner friction
	// TODO: This is very sensitive to numerical instabilities for mid-to-high
	// eta values. We might want to prevent a sign change of either F or V induced
	// by this factor.
	// TODO: Don't apply inner friction for segments connected to fixed rope ends?
	C4Real fx = (prev->GetVx() - next->GetVx()) * eta;
	C4Real fy = (prev->GetVy() - next->GetVy()) * eta;
	prev->AddForce(-fx, -fy);
	next->AddForce(fx, fy);

	// Could add air/water friction here

	// TODO: Apply gravity separately. This is applied twice now for
	// non-end rope segments!

	// Don't apply gravity to objects since it's applied already in C4Object execution.
	prev->AddForce(Fix0, (prev->GetObject() ? Fix0 : prev->GetMass() * ::Landscape.Gravity/5));
	next->AddForce(Fix0, (prev->GetObject() ? Fix0 : next->GetMass() * ::Landscape.Gravity/5));
}

void C4Rope::Execute()
{
	C4Real dt = itofix(1, NumIterations);
	for(unsigned int i = 0; i < NumIterations; ++i)
	{
		// Execute auto-segmentation
		DoAutoSegmentation(Front, Front->Next, FrontAutoSegmentation);
		DoAutoSegmentation(Back, Back->Prev, BackAutoSegmentation);

		// Reset previous forces
#ifdef C4ROPE_DRAW_DEBUG
		for(C4RopeElement* cur = Front; cur != NULL; cur = cur->Next)
			cur->fcx = cur->fcy = Fix0;
#endif

		// Compute inter-rope forces
		for(C4RopeElement* cur = Front; cur->Next != NULL; cur = cur->Next)
			Solve(cur, cur->Next);

		// Front/BackPull
		if(FrontPull != Fix0)
		{
			// Pull at all elements that are near to the front element
			C4RopeElement* cur = Front;
			C4Real d;
			do
			{
				cur = cur->Next;
				const C4Real dx = cur->GetX() - Front->GetX();
				const C4Real dy = cur->GetY() - Front->GetY();
				d = Len(dx, dy);
				if(d != Fix0)
					cur->AddForce(-dx/d * FrontPull, -dy/d * FrontPull);
			} while(cur->Next != NULL && d < itofix(5,10*l));
		}

		if(BackPull != Fix0)
		{
			// Pull at all elements that are near to the back element
			C4RopeElement* cur = Back;
			C4Real d;
			do
			{
				cur = cur->Prev;
				const C4Real dx = cur->GetX() - Back->GetX();
				const C4Real dy = cur->GetY() - Back->GetY();
				d = Len(dx, dy);
				if(d != Fix0)
					cur->AddForce(-dx/d * BackPull, -dy/d * BackPull);
			} while(cur->Prev != NULL && d < itofix(5,10*l));
		}

		// Apply forces
		for(C4RopeElement* cur = Front; cur != NULL; cur = cur->Next)
			cur->Execute(this, dt);
	}

	// Insert/remove links between rope elements. We rely that object movement
	// is executed before rope movement at this point, so this needs to stay in
	// sync with C4Game::Execute().
	for(C4RopeElement* cur = Front; cur != NULL; cur = cur->Next)
	{
		int insert_x, insert_y;
		if(cur->Prev && cur->InsertLinkPosition(cur->oldx, cur->oldy, cur->GetX(), cur->GetY(), cur->GetPrevLinkX(), cur->GetPrevLinkY(), insert_x, insert_y))
			cur->InsertLink(cur->Prev, cur, insert_x, insert_y);
		if(cur->Next && cur->InsertLinkPosition(cur->oldx, cur->oldy, cur->GetX(), cur->GetY(), cur->GetNextLinkX(), cur->GetNextLinkY(), insert_x, insert_y))
			cur->InsertLink(cur, cur->Next, insert_x, insert_y);

		// TODO: removal
	}

	// Update old coordinates for next iteration. Don't do this in the loop above,
	// so that GetPrevLinkX/GetPrevLinkY still return the old values.
	for(C4RopeElement* cur = Front; cur != NULL; cur = cur->Next)
	{
		cur->oldx = cur->GetX();
		cur->oldy = cur->GetY();
	}
}

void C4Rope::ClearPointers(C4Object* obj)
{
	for(C4RopeElement* cur = Front; cur != NULL; cur = cur->Next)
	{
		if(cur->GetObject() == obj)
		{
			cur->x = obj->fix_x;
			cur->y = obj->fix_y;
			cur->vx = obj->xdir;
			cur->vy = obj->ydir;
			cur->m = obj->Mass;

			cur->Object = NULL;
		}
	}
}

// TODO: Move this to StdGL
void C4Rope::Draw(C4TargetFacet& cgo, C4BltTransform* pTransform)
{
#ifndef C4ROPE_DRAW_DEBUG
	Vertex Tmp[4];
	DrawVertex* Vertices = new DrawVertex[SegmentCount*2+4]; // TODO: Use a vbo and map it into memory instead?

	VertexPos(Vertices[0], Vertices[1], Tmp[0], Tmp[1],
	          Vertex(fixtof(Front->GetX()), fixtof(Front->GetY())),
	          Vertex(fixtof(Front->Next->GetX()), fixtof(Front->Next->GetY())), Width);

	Vertices[0].u = 0.0f;
	Vertices[0].v = 0.0f;
	Vertices[1].u = 1.0f;
	Vertices[1].v = 0.0f;

	const float rsl = 1.0f/Width * Graphics->GetBitmap()->Wdt / Graphics->GetBitmap()->Hgt; // rope segment length mapped to Gfx bitmap
	float accl = 0.0f;

	unsigned int i = 2;
	bool parity = true;
	for(C4RopeElement* cur = Front->Next; cur->Next != NULL; cur = cur->Next, i += 2)
	{
		Vertex v1(fixtof(cur->GetX()),
		          fixtof(cur->GetY()));
		Vertex v2(fixtof(cur->Next->GetX()),// ? cur->Next->GetX() : Back->GetX()), 
		          fixtof(cur->Next->GetY()));// ? cur->Next->GetY() : Back->GetY()));
		Vertex v3(fixtof(cur->Prev->GetX()),// ? cur->Prev->GetX() : Front->GetX()),
		          fixtof(cur->Prev->GetY()));// ? cur->Prev->GetY() : Front->GetY()));

		//const C4Real l = GetL(cur->Prev, cur);
		//const float rsl = fixtof(l)/fixtof(Width) * Graphics->GetBitmap()->Wdt / Graphics->GetBitmap()->Hgt; // rope segment length mapped to Gfx bitmap

		// Parity -- parity swaps for each pointed angle (<90 deg)
		float cx = v1.x - v3.x;
		float cy = v1.y - v3.y;
		float ex = v1.x - v2.x;
		float ey = v1.y - v2.y;
		
		// TODO: Another way to draw this would be to insert a "pseudo" segment so that there are no pointed angles at all
		if(cx*ex+cy*ey > 0)
			parity = !parity;

		// Obtain vertex positions
		if(parity)
			VertexPos(Tmp[2], Tmp[3], Vertices[i+2], Vertices[i+3], v1, v2, Width);
		else
			VertexPos(Tmp[3], Tmp[2], Vertices[i+3], Vertices[i+2], v1, v2, Width);

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
		Vertices[i  ].x = ( (Tmp[2].x + Tmp[3].x)/2.0f) - BoundBy((Tmp[3].x - Tmp[2].x)*d, -Width, Width)/2.0f;
		Vertices[i  ].y = ( (Tmp[2].y + Tmp[3].y)/2.0f) - BoundBy((Tmp[3].y - Tmp[2].y)*d, -Width, Width)/2.0f;
		Vertices[i+1].x = ( (Tmp[2].x + Tmp[3].x)/2.0f) + BoundBy((Tmp[3].x - Tmp[2].x)*d, -Width, Width)/2.0f;
		Vertices[i+1].y = ( (Tmp[2].y + Tmp[3].y)/2.0f) + BoundBy((Tmp[3].y - Tmp[2].y)*d, -Width, Width)/2.0f;

		accl += fixtof(GetL(cur->Prev, cur));
		Vertices[i].u = 0.0f; //parity ? 0.0f : 1.0f;
		Vertices[i].v = accl * rsl;
		Vertices[i+1].u = 1.0f; //parity ? 1.0f : 0.0f;
		Vertices[i+1].v = accl * rsl;

		Tmp[0] = Vertices[i+2];
		Tmp[1] = Vertices[i+3];
	}

	accl += fixtof(GetL(Back->Prev, Back));
	Vertices[i].u = 0.0f; //parity ? 0.0f : 1.0f;
	Vertices[i].v = accl * rsl;
	Vertices[i+1].u = 1.0f; //parity ? 1.0f : 0.0f;
	Vertices[i+1].v = accl * rsl;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, (*Graphics->GetBitmap()->ppTex)->texName);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glVertexPointer(2, GL_FLOAT, sizeof(DrawVertex), &Vertices->x);
	glTexCoordPointer(2, GL_FLOAT, sizeof(DrawVertex), &Vertices->u);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDrawArrays(GL_QUAD_STRIP, 0, SegmentCount*2+4);

	glDisable(GL_TEXTURE_2D);
	//glDisable(GL_BLEND);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	delete[] Vertices;

#else
	// Debug:
	for(C4RopeElement* cur = Front; cur != NULL; cur = cur->Next)
	{
		if(!cur->GetObject())
		{
			glBegin(GL_QUADS);
			glColor3f(1.0f, 0.0f, 0.0f);
			glVertex2f(fixtof(cur->x)-1.5f, fixtof(cur->y)-1.5f);
			glVertex2f(fixtof(cur->x)-1.5f, fixtof(cur->y)+1.5f);
			glVertex2f(fixtof(cur->x)+1.5f, fixtof(cur->y)+1.5f);
			glVertex2f(fixtof(cur->x)+1.5f, fixtof(cur->y)-1.5f);
			glEnd();
		}

		// Draw links
		for(C4RopeLink* link = cur->FirstLink; link != NULL; link = link->Next)
		{
			glBegin(GL_QUADS);
			glColor3f(1.0f, 1.0f, 1.0f);
			glVertex2f(fixtof(cur->x)-1.0f, fixtof(cur->y)-1.0f);
			glVertex2f(fixtof(cur->x)-1.0f, fixtof(cur->y)+1.0f);
			glVertex2f(fixtof(cur->x)+1.0f, fixtof(cur->y)+1.0f);
			glVertex2f(fixtof(cur->x)+1.0f, fixtof(cur->y)-1.0f);
			glEnd();
		}

		const float vx = fixtof(cur->GetVx());
		const float vy = fixtof(cur->GetVy());
		const float v = sqrt(vx*vx + vy*vy);
		if(v > 0.1)
		{
			glBegin(GL_TRIANGLES);
			glColor3f(BoundBy(v/2.5f, 0.0f, 1.0f), 0.0f, 1.0f);
			glVertex2f(fixtof(cur->GetX()) + vx/v*4.0f, fixtof(cur->GetY()) + vy/v*4.0f);
			glVertex2f(fixtof(cur->GetX()) - vy/v*1.5f, fixtof(cur->GetY()) + vx/v*1.5f);
			glVertex2f(fixtof(cur->GetX()) + vy/v*1.5f, fixtof(cur->GetY()) - vx/v*1.5f);
			glEnd();
		}

		const float fx = fixtof(cur->fcx);
		const float fy = fixtof(cur->fcy);
		const float f = sqrt(fx*fx + fy*fy);
		if(f > 0.1)
		{
			glBegin(GL_TRIANGLES);
			glColor3f(0.0f, 1.0f, BoundBy(v/2.5f, 0.0f, 1.0f));
			glVertex2f(fixtof(cur->GetX()) + fx/f*4.0f, fixtof(cur->GetY()) + fy/f*4.0f);
			glVertex2f(fixtof(cur->GetX()) - fy/f*1.5f, fixtof(cur->GetY()) + fx/f*1.5f);
			glVertex2f(fixtof(cur->GetX()) + fy/f*1.5f, fixtof(cur->GetY()) - fx/f*1.5f);
			glEnd();
		}

		const float rx = fixtof(cur->rx);
		const float ry = fixtof(cur->ry);
		const float r = sqrt(rx*rx + ry*ry);
		if(r > 0.1)
		{
			glBegin(GL_TRIANGLES);
			glColor3f(0.0f, BoundBy(r/2.5f, 0.0f, 1.0f), 1.0f);
			glVertex2f(fixtof(cur->x) + rx/r*4.0f, fixtof(cur->y) + ry/r*4.0f);
			glVertex2f(fixtof(cur->x) - ry/r*1.5f, fixtof(cur->y) + rx/r*1.5f);
			glVertex2f(fixtof(cur->x) + ry/r*1.5f, fixtof(cur->y) - rx/r*1.5f);
			glEnd();
		}
	}
#endif
}

C4RopeList::C4RopeList()
{
	for(unsigned int i = 0; i < Ropes.size(); ++i)
		delete Ropes[i];
}

C4Rope* C4RopeList::CreateRope(C4Object* first_obj, C4Object* second_obj, C4Real segment_length, C4DefGraphics* graphics)
{
	Ropes.push_back(new C4Rope(RopeAul.GetPropList(), first_obj, second_obj, segment_length, graphics));
	return Ropes.back();
}

void C4RopeList::RemoveRope(C4Rope* rope)
{
	for(std::vector<C4Rope*>::iterator iter = Ropes.begin(); iter != Ropes.end(); ++iter)
	{
		if(*iter == rope)
		{
			Ropes.erase(iter);
			delete rope;
			break;
		}
	}
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

void C4RopeList::ClearPointers(C4Object* obj)
{
	for(unsigned int i = 0; i < Ropes.size(); ++i)
		Ropes[i]->ClearPointers(obj);
}
