/*
* OpenClonk, http://www.openclonk.org
*
* Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
* Copyright (c) 2013, The OpenClonk Team and contributors
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

/* Editable shapes in the viewports (like e.g. AI guard range rectangles) */

#include "C4Include.h"
#include "graphics/C4FacetEx.h"
#include "graphics/C4Draw.h"
#include "object/C4Object.h"
// See C4ConsoleQt.cpp on include order
#include "editor/C4Console.h"
#include "editor/C4ConsoleQtState.h"
#include "editor/C4ConsoleQtShapes.h"

/* Generic shape */

C4ConsoleQtShape::C4ConsoleQtShape(C4Object *for_obj, C4PropList *props, const class C4PropertyDelegateShape *parent_delegate)
	: is_relative(true), dragging_border(-1), border_color(0xffff0000), parent_delegate(parent_delegate)
{
	rel_obj.SetPropList(for_obj);
	if (props)
	{
		is_relative = props->GetPropertyBool(P_Relative, is_relative);
		border_color = props->GetPropertyInt(P_Color) | 0xff000000;
	}
}

uint32_t C4ConsoleQtShape::GetBorderColor(int32_t border_index, bool dragging_border_is_bitmask, uint32_t default_color) const
{
	// Return shape color, or dragged border color if index is the border currently being dragged
	if (IsDragging())
		if ((dragging_border == border_index) || (dragging_border_is_bitmask && (dragging_border & border_index)))
			return 0xffffffff;
	return default_color ? default_color : border_color;
}

int32_t C4ConsoleQtShape::AbsX(int32_t rel_x) const
{
	if (is_relative)
	{
		C4Object *obj = rel_obj.getObj();
		if (obj) rel_x += obj->GetX();
	}
	return rel_x;
}

int32_t C4ConsoleQtShape::AbsY(int32_t rel_y) const
{
	if (is_relative)
	{
		C4Object *obj = rel_obj.getObj();
		if (obj) rel_y += obj->GetY();
	}
	return rel_y;
}

void C4ConsoleQtShape::StopDragging()
{
	// Reset drag state and emit signal to send updated value
	dragging_border = -1;
	emit ShapeDragged();
}


/* Rectangular shape*/

C4ConsoleQtRect::C4ConsoleQtRect(C4Object *for_obj, C4PropList *props, const class C4PropertyDelegateShape *parent_delegate)
	: C4ConsoleQtShape(for_obj, props, parent_delegate), left(0), top(0), right(10), bottom(10), store_as_proplist(false), properties_lowercase(false)
{
	// Def props
	if (props)
	{
		C4String *storage = props->GetPropertyStr(P_Storage);
		if (storage)
		{
			if (storage == &::Strings.P[P_proplist])
				properties_lowercase = store_as_proplist = true;
			else if (storage == &::Strings.P[P_Proplist])
				store_as_proplist = true;
		}
	}
}

bool C4ConsoleQtRect::IsHit(int32_t x, int32_t y, int32_t hit_range, Qt::CursorShape *drag_cursor, int32_t *drag_border, bool shift_down, bool ctrl_down)
{
	// Current border pos
	int32_t left = AbsX(this->left), top = AbsY(this->top);
	int32_t right = AbsX(this->right), bottom = AbsY(this->bottom);
	// Distance to each border
	int32_t dleft = Abs<int32_t>(left - x);
	int32_t dtop = Abs<int32_t>(top - y);
	int32_t dright = Abs<int32_t>(right - x);
	int32_t dbottom = Abs<int32_t>(bottom - y);
	// In box at all?
	if (x < left - hit_range || y < top - hit_range || x > right + hit_range || y > bottom + hit_range)
		return false;
	// Border hit?
	bool hit_left = (dleft <= hit_range && dleft < dright);
	bool hit_top = (dtop <= hit_range && dtop < dbottom);
	bool hit_right = (!hit_left && dright <= hit_range);
	bool hit_bottom = (!hit_top && dbottom <= hit_range);
	// Compose cursor and drag border
	int32_t idrag_border = (hit_left * CNAT_Left) + (hit_top * CNAT_Top) + (hit_right * CNAT_Right) + (hit_bottom * CNAT_Bottom);
	if (idrag_border) *drag_border = idrag_border;
	if (hit_left || hit_right)
		if (hit_top || hit_bottom)
			*drag_cursor = (hit_left == hit_top) ? Qt::SizeFDiagCursor : Qt::SizeBDiagCursor;
		else
			*drag_cursor = Qt::SizeHorCursor;
	else if (hit_top || hit_bottom)
		*drag_cursor = Qt::SizeVerCursor;
	return !!idrag_border;
}

void C4ConsoleQtRect::Draw(class C4TargetFacet &cgo, float line_width)
{
	float left = float(AbsX(this->left)) + cgo.X - cgo.TargetX;
	float top = float(AbsY(this->top)) + cgo.Y - cgo.TargetY;
	float right = float(AbsX(this->right)) + cgo.X - cgo.TargetX;
	float bottom = float(AbsY(this->bottom)) + cgo.Y - cgo.TargetY;
	pDraw->DrawLineDw(cgo.Surface, left, top, right, top, GetBorderColor(CNAT_Top, true), line_width);
	pDraw->DrawLineDw(cgo.Surface, right, top, right, bottom, GetBorderColor(CNAT_Right, true), line_width);
	pDraw->DrawLineDw(cgo.Surface, right, bottom, left, bottom, GetBorderColor(CNAT_Bottom, true), line_width);
	pDraw->DrawLineDw(cgo.Surface, left, bottom, left, top, GetBorderColor(CNAT_Left, true), line_width);
}

void C4ConsoleQtRect::Drag(int32_t x, int32_t y, int32_t dx, int32_t dy, int32_t hit_range, Qt::CursorShape *drag_cursor)
{
	if (dragging_border & CNAT_Left) left += dx;
	if (dragging_border & CNAT_Top) top += dy;
	if (dragging_border & CNAT_Right) right += dx;
	if (dragging_border & CNAT_Bottom) bottom += dy;
	if (left > right) std::swap(left, right);
	if (top > bottom) std::swap(top, bottom);
}

void C4ConsoleQtRect::SetValue(const C4Value &val)
{
	// Expect rect to be given as proplist with properties X, Y, Wdt, Hgt or array with elements [left,top,width,height]
	if (store_as_proplist)
	{
		C4PropList *vprops = val.getPropList();
		if (vprops)
		{
			left = vprops->GetPropertyInt(properties_lowercase ? P_x : P_X);
			top = vprops->GetPropertyInt(properties_lowercase ? P_y : P_Y);
			right = left + vprops->GetPropertyInt(properties_lowercase ? P_wdt : P_Wdt) - 1;
			bottom = top + vprops->GetPropertyInt(properties_lowercase ? P_hgt : P_Hgt) - 1;
		}
	}
	else
	{
		C4ValueArray *varr = val.getArray();
		if (varr && varr->GetSize() >= 4)
		{
			left = varr->GetItem(0).getInt();
			top = varr->GetItem(1).getInt();
			right = left + varr->GetItem(2).getInt() - 1; // right/bottom borders are drawn inclusively
			bottom = top + varr->GetItem(3).getInt() - 1;
		}
	}
}

C4Value C4ConsoleQtRect::GetValue() const
{
	// Return array or proplist: Convert left/top/right/bottom (inclusive) to left/top/width/height
	if (store_as_proplist)
	{
		C4PropList *pos_proplist = new C4PropListScript();
		pos_proplist->SetProperty(properties_lowercase ? P_x : P_X, C4VInt(left));
		pos_proplist->SetProperty(properties_lowercase ? P_y : P_Y, C4VInt(top));
		pos_proplist->SetProperty(properties_lowercase ? P_wdt : P_Wdt, C4VInt(right - left + 1));
		pos_proplist->SetProperty(properties_lowercase ? P_hgt : P_Hgt, C4VInt(bottom - top + 1));
		return C4VPropList(pos_proplist);
	}
	else
	{
		C4ValueArray *pos_array = new C4ValueArray(4);
		pos_array->SetItem(0, C4VInt(left));
		pos_array->SetItem(1, C4VInt(top));
		pos_array->SetItem(2, C4VInt(right - left + 1));
		pos_array->SetItem(3, C4VInt(bottom - top + 1));
		return C4VArray(pos_array);
	}
}


/* Circle shape */

C4ConsoleQtCircle::C4ConsoleQtCircle(class C4Object *for_obj, C4PropList *props, const class C4PropertyDelegateShape *parent_delegate)
	: C4ConsoleQtShape(for_obj, props, parent_delegate), radius(10), cx(0), cy(0), can_move_center(false)
{
	if (props)
	{
		can_move_center = props->GetPropertyBool(P_CanMoveCenter);
	}
}

bool C4ConsoleQtCircle::IsHit(int32_t x, int32_t y, int32_t hit_range, Qt::CursorShape *drag_cursor, int32_t *drag_border, bool shift_down, bool ctrl_down)
{
	// Get relative circle center pos
	x -= AbsX(cx);
	y -= AbsY(cy);
	int32_t r = x*x + y*y;
	// Is on circle border? (Higher priority than center to allow resizing circle from 0 radius)
	if (Inside<int32_t>(r, (radius - hit_range)*(radius - hit_range), (radius + hit_range)*(radius + hit_range)))
	{
		// Cursor by position on 60 deg circle segments
		if (x * 58 / 100 / (y+!y)) // tan(30) ~= 0.58
			*drag_cursor = Qt::CursorShape::SizeHorCursor;
		else if (y * 58 / 100 / (x+!x))
			*drag_cursor = Qt::CursorShape::SizeVerCursor;
		else if (x*y > 0)
			*drag_cursor = Qt::CursorShape::SizeFDiagCursor;
		else
			*drag_cursor = Qt::CursorShape::SizeBDiagCursor;
		*drag_border = 0;
		return true;
	}
	// Circle center?
	if (can_move_center && r <= hit_range*hit_range)
	{
		*drag_cursor = Qt::CursorShape::SizeAllCursor;
		*drag_border = 1;
		return true;
	}
	return false;
}

void C4ConsoleQtCircle::Draw(class C4TargetFacet &cgo, float line_width)
{
	// Circle
	pDraw->DrawCircleDw(cgo.Surface, AbsX(cx) + cgo.X - cgo.TargetX, AbsY(cy) + cgo.Y - cgo.TargetY, radius, GetBorderColor(0, false), line_width);
	// Center if moveable
	if (can_move_center)
		pDraw->DrawCircleDw(cgo.Surface, AbsX(cx) + cgo.X - cgo.TargetX, AbsY(cy) + cgo.Y - cgo.TargetY, line_width*3, GetBorderColor(1, false), line_width);
}

void C4ConsoleQtCircle::C4ConsoleQtCircle::Drag(int32_t x, int32_t y, int32_t dx, int32_t dy, int32_t hit_range, Qt::CursorShape *drag_cursor)
{
	if (dragging_border == 0)
	{
		x -= AbsX(cx);
		y -= AbsY(cy);
		radius = int32_t(sqrt(double(x*x + y*y)));
	}
	else if (dragging_border == 1)
	{
		cx += dx;
		cy += dy;
	}
}

void C4ConsoleQtCircle::SetValue(const C4Value &val)
{
	// If center is moveable, expect value as [radius, center_x, center_y]
	// Otherwise just radius
	if (can_move_center)
	{
		C4ValueArray *aval = val.getArray();
		if (aval && aval->GetSize() == 3)
		{
			radius = aval->GetItem(0).getInt();
			cx = aval->GetItem(1).getInt();
			cy = aval->GetItem(2).getInt();
		}
	}
	else
	{
		radius = val.getInt();
	}
}

C4Value C4ConsoleQtCircle::GetValue() const
{
	// Return single value for non-center-adjustable circles; return [radius, cx, cy] otherwise
	if (can_move_center)
	{
		C4ValueArray *pos_array = new C4ValueArray(3);
		pos_array->SetItem(0, C4VInt(radius));
		pos_array->SetItem(1, C4VInt(cx));
		pos_array->SetItem(2, C4VInt(cy));
		return C4VArray(pos_array);
	}
	else
	{
		return C4VInt(radius);
	}
}


/* Point shape */

C4ConsoleQtPoint::C4ConsoleQtPoint(class C4Object *for_obj, C4PropList *props, const class C4PropertyDelegateShape *parent_delegate)
	: C4ConsoleQtShape(for_obj, props, parent_delegate), cx(0), cy(0)
{
}

bool C4ConsoleQtPoint::IsHit(int32_t x, int32_t y, int32_t hit_range, Qt::CursorShape *drag_cursor, int32_t *drag_border, bool shift_down, bool ctrl_down)
{
	// Get relative circle center pos
	x -= AbsX(cx);
	y -= AbsY(cy);
	int32_t r = x*x + y*y;
	// Hits point?
	if (r <= hit_range*hit_range*6)
	{
		*drag_cursor = Qt::CursorShape::SizeAllCursor;
		*drag_border = 0;
		return true;
	}
	return false;
}

void C4ConsoleQtPoint::Draw(class C4TargetFacet &cgo, float line_width)
{
	// Circle with cross inside
	uint32_t clr = GetBorderColor(0, false);
	float d = line_width * 3;
	float dc = sqrtf(2) * d;
	int32_t x = AbsX(cx) + cgo.X - cgo.TargetX;
	int32_t y = AbsY(cy) + cgo.Y - cgo.TargetY;
	pDraw->DrawLineDw(cgo.Surface, x - d, y - d, x + d, y + d, clr, line_width);
	pDraw->DrawLineDw(cgo.Surface, x - d, y + d, x + d, y - d, clr, line_width);
	pDraw->DrawCircleDw(cgo.Surface, x, y, dc, clr, line_width);
}

void C4ConsoleQtPoint::Drag(int32_t x, int32_t y, int32_t dx, int32_t dy, int32_t hit_range, Qt::CursorShape *drag_cursor)
{
	cx += dx;
	cy += dy;
}

void C4ConsoleQtPoint::SetValue(const C4Value &val)
{
	// Expect value as [x,y]
	C4ValueArray *aval = val.getArray();
	if (aval && aval->GetSize() == 2)
	{
		cx = aval->GetItem(0).getInt();
		cy = aval->GetItem(1).getInt();
	}
}

C4Value C4ConsoleQtPoint::GetValue() const
{
	// Return [cx, cy]
	C4ValueArray *pos_array = new C4ValueArray(2);
	pos_array->SetItem(0, C4VInt(cx));
	pos_array->SetItem(1, C4VInt(cy));
	return C4VArray(pos_array);
}


/* Graph */

bool C4ConsoleQtGraph::Edge::connects_to(int32_t vertex_index) const
{
	// Check if this edge connects to given vertex
	return vertex_indices[0] == vertex_index || vertex_indices[1] == vertex_index;
}

bool C4ConsoleQtGraph::Edge::connects_to(int32_t vertex_index, int32_t *idx) const
{
	// Check if this edge connects to given vertex. If so, put vertex_index index into *idx
	assert(idx);
	if (vertex_indices[0] == vertex_index)
	{
		*idx = 0;
	}
	else if (vertex_indices[1] == vertex_index)
	{
		*idx = 1;
	}
	else
	{
		return false;
	}
	return true;
}

C4ConsoleQtGraph::C4ConsoleQtGraph(C4Object *for_obj, C4PropList *props, const class C4PropertyDelegateShape *parent_delegate)
	: C4ConsoleQtShape(for_obj, props, parent_delegate), store_as_proplist(false), allow_edge_selection(false)
{
	// Def props
	if (props)
	{
		store_as_proplist = (props->GetPropertyStr(P_Storage) == &::Strings.P[P_Proplist]);
	}
}

bool C4ConsoleQtGraph::IsHit(int32_t x, int32_t y, int32_t hit_range, Qt::CursorShape *drag_cursor, int32_t *drag_border, bool shift_down, bool ctrl_down)
{
	// Check hit on vertices
	int32_t i = 0, best_hit_range = hit_range*hit_range * 6;
	bool has_hit = false;
	for (const Vertex &vtx : vertices)
	{
		int32_t dx = x - AbsX(vtx.x);
		int32_t dy = y - AbsY(vtx.y);
		int32_t r = dx*dx + dy*dy;
		// Is close to vertex / closer than previous hit?
		if (r <= best_hit_range)
		{
			if (IsVertexHit(i, drag_cursor, shift_down, ctrl_down))
			{
				*drag_border = VertexToDragBorder(i);
				best_hit_range = r;
				has_hit = true;
			}
		}
		++i;
	}
	// Check hit on edge if edge selection is possible
	if (!has_hit && (allow_edge_selection || shift_down || ctrl_down))
	{
		best_hit_range = hit_range*hit_range;
		i = 0;
		for (const Edge &edge : edges)
		{
			// Get affected vertices
			assert(edge.vertex_indices[0] >= 0 && edge.vertex_indices[1] >= 0);
			assert(edge.vertex_indices[0] < vertices.size() && edge.vertex_indices[1] < vertices.size());
			const Vertex &v0 = vertices[edge.vertex_indices[0]];
			const Vertex &v1 = vertices[edge.vertex_indices[1]];
			// Relative edge pos
			int32_t dx0 = x - AbsX(v0.x);
			int32_t dy0 = y - AbsY(v0.y);
			int32_t dx1 = v1.x - v0.x;
			int32_t dy1 = v1.y - v0.y;
			// Check if within line range
			int32_t d = dx0 * dx1 + dy0 * dy1;
			if (d > 0 && d < dx1 * dx1 + dy1 * dy1)
			{
				// Get squared distance from edge
				d = dx1 * dy0 - dy1 * dx0;
				d = d * d / (dx1 * dx1 + dy1 * dy1);
				// In hit range?
				if (d <= best_hit_range)
				{
					if (IsEdgeHit(i, drag_cursor, shift_down, ctrl_down))
					*drag_border = EdgeToDragBorder(i);
					best_hit_range = d;
					has_hit = true;
				}
			}
			++i;
		}
	}
	return has_hit;
}

bool C4ConsoleQtGraph::IsVertexHit(int32_t vertex_index, Qt::CursorShape *drag_cursor, bool shift_down, bool ctrl_down)
{
	if (shift_down && !ctrl_down)
	{
		// Insert vertex here
		*drag_cursor = Qt::CursorShape::DragCopyCursor;
	}
	else if (ctrl_down && !shift_down)
	{
		// Remove this vertex (unless it's the last, which cannot be removed)
		if (vertices.size() == 1)
		{
			*drag_cursor = Qt::CursorShape::ForbiddenCursor;
		}
		else
		{
			*drag_cursor = Qt::CursorShape::DragMoveCursor;
		}
	}
	else
	{
		// Normal dragging
		*drag_cursor = Qt::CursorShape::SizeAllCursor;
	}
	return true;
}

bool C4ConsoleQtGraph::IsEdgeHit(int32_t edge_index, Qt::CursorShape *drag_cursor, bool shift_down, bool ctrl_down)
{
	if (shift_down && !ctrl_down)
	{
		// Insert vertex here
		*drag_cursor = Qt::CursorShape::DragCopyCursor;
	}
	else if (ctrl_down && !shift_down)
	{
		// Remove this edge
		*drag_cursor = Qt::CursorShape::DragMoveCursor;
	}
	else if (allow_edge_selection)
	{
		// Normal selection
		*drag_cursor = Qt::CursorShape::PointingHandCursor; // can select, but cannot move
	}
	else
	{
		// Nothing to do with the edge. Selection disabled.
		return false;
	}
	return true;
}

void C4ConsoleQtGraph::Draw(class C4TargetFacet &cgo, float line_width)
{
	// Draw edges as lines
	int32_t i = 0;
	for (const Edge &edge : edges)
	{
		uint32_t clr = GetBorderColor(EdgeToDragBorder(i++), false, edge.color);
		assert(edge.vertex_indices[0] >= 0 && edge.vertex_indices[1] >= 0);
		assert(edge.vertex_indices[0] < vertices.size() && edge.vertex_indices[1] < vertices.size());
		const Vertex &v0 = vertices[edge.vertex_indices[0]];
		const Vertex &v1 = vertices[edge.vertex_indices[1]];
		pDraw->DrawLineDw(cgo.Surface,
			AbsX(v0.x) + cgo.X - cgo.TargetX,
			AbsY(v0.y) + cgo.Y - cgo.TargetY,
			AbsX(v1.x) + cgo.X - cgo.TargetX,
			AbsY(v1.y) + cgo.Y - cgo.TargetY,
			clr, line_width * edge.line_thickness);
	}
	// Draw vertices as circles with cross inside
	i = 0;
	for (const Vertex &vtx : vertices)
	{
		uint32_t clr = GetBorderColor(VertexToDragBorder(i++), false, vtx.color);
		float d = line_width * 3;
		float dc = sqrtf(2) * d;
		int32_t x = AbsX(vtx.x) + cgo.X - cgo.TargetX;
		int32_t y = AbsY(vtx.y) + cgo.Y - cgo.TargetY;
		pDraw->DrawLineDw(cgo.Surface, x - d, y - d, x + d, y + d, clr, line_width);
		pDraw->DrawLineDw(cgo.Surface, x - d, y + d, x + d, y - d, clr, line_width);
		pDraw->DrawCircleDw(cgo.Surface, x, y, dc, clr, line_width);
	}
}

bool C4ConsoleQtGraph::StartDragging(int32_t border, int32_t x, int32_t y, bool shift_down, bool ctrl_down)
{
	assert(border != -1);
	drag_snap_offset_x = drag_snap_offset_y = 0;
	drag_snapped = false;
	drag_snap_vertex = -1;
	drag_source_vertex_index = -1;
	if (shift_down && !ctrl_down)
	{
		// Shift: Insert vertex
		if (IsEdgeDrag(border))
		{
			// Insert on edge
			dragging_border = VertexToDragBorder(InsertVertexOnEdge(DragBorderToEdge(border), x - AbsX(), y - AbsY()));
		}
		else
		{
			// Insert from other vertex
			drag_source_vertex_index = DragBorderToVertex(border);
			dragging_border = VertexToDragBorder(InsertVertexOnVertex(drag_source_vertex_index, x - AbsX(), y - AbsY()));
		}
		// Start dragging
		return true;
	}
	else if (ctrl_down && !shift_down)
	{
		// Ctrl: Delete vertex or edge
		if (IsEdgeDrag(border))
		{
			// Delete edge
			RemoveEdge(DragBorderToEdge(border));
		}
		else
		{
			// Remove vertex unless it is the last one
			// If there's only one vertex left, just keep it
			if (vertices.size() > 1) RemoveVertex(DragBorderToVertex(border), true);
		}
		// Notify script
		emit ShapeDragged();
		// Do not start dragging
		return false;
	}
	else if (!shift_down && !ctrl_down)
	{
		// Regular dragging of vertices
		if (IsVertexDrag(border))
		{
			return C4ConsoleQtShape::StartDragging(border, x, y, shift_down, ctrl_down);
		}
		else
		{
			// No dragging of edges
			return false;
		}
	}
	else
	{
		// Unknown modifiers
		return false;
	}
}

void C4ConsoleQtGraph::Drag(int32_t x, int32_t y, int32_t dx, int32_t dy, int32_t hit_range, Qt::CursorShape *drag_cursor)
{
	// Dragging: Only vertices
	if (IsVertexDrag(dragging_border) && DragBorderToVertex(dragging_border) < vertices.size())
	{
		int32_t dragged_vertex_index = DragBorderToVertex(dragging_border);
		Vertex &dragged_vertex = vertices[dragged_vertex_index];
		// Regular vertex movement
		dx -= drag_snap_offset_x;
		dy -= drag_snap_offset_y;
		dragged_vertex.x += dx;
		dragged_vertex.y += dy;
		// Handle snap to combine with other vertices
		if (!IsPolyline())
		{
			int32_t i = 0;
			drag_snap_vertex = -1;
			int32_t best_hit_range_sq = hit_range * hit_range * 4;
			for (Vertex &check_vertex : vertices)
			{
				if (i != dragged_vertex_index && i != drag_source_vertex_index)
				{
					int32_t cdx = check_vertex.x - dragged_vertex.x - dx;
					int32_t cdy = check_vertex.y - dragged_vertex.y - dy;
					int32_t cdsq = cdx*cdx + cdy*cdy;
					if (cdsq <= best_hit_range_sq)
					{
						drag_snap_vertex = i;
						best_hit_range_sq = cdsq;
					}
				}
				++i;
			}
			// Snapped? Move vertex to snap vertex then
			drag_snapped = (drag_snap_vertex >= 0);
			if (drag_snapped)
			{
				drag_snap_offset_x = vertices[drag_snap_vertex].x - dragged_vertex.x;
				drag_snap_offset_y = vertices[drag_snap_vertex].y - dragged_vertex.y;
				dragged_vertex.x += drag_snap_offset_x;
				dragged_vertex.y += drag_snap_offset_y;
				*drag_cursor = Qt::CursorShape::DragMoveCursor;
			}
			else
			{
				drag_snap_offset_x = drag_snap_offset_y = 0;
				*drag_cursor = Qt::CursorShape::SizeAllCursor;
			}
		}
	}
}

void C4ConsoleQtGraph::StopDragging()
{
	// Is it a vertex recombination?
	if (IsVertexDrag(dragging_border) && drag_snapped)
	{
		int32_t dragged_vertex = DragBorderToVertex(dragging_border);
		if (dragged_vertex && dragged_vertex != drag_snap_vertex)
		{
			// dragged_vertex is to be merged into drag_snap_vertex (keeping drag_snap_vertex)
			// find all edge targets already existing for drag_snap_vertex (this may include dragged_vertex)
			std::set<int32_t> vertices_connected_to_snap_vertex;
			int32_t idx;
			for (Edge &edge : edges)
			{
				if (edge.connects_to(drag_snap_vertex, &idx))
				{
					vertices_connected_to_snap_vertex.insert(edge.vertex_indices[!idx]);
				}
			}
			// make sure that any connection from dragged_vertex to drag_snap_vertex will be excluded by the check
			vertices_connected_to_snap_vertex.insert(drag_snap_vertex);
			// move all connections that did not exist yet from dragged_vertex to drag_snap_vertex
			for (Edge &edge : edges)
			{
				if (edge.connects_to(dragged_vertex, &idx))
				{
					if (!vertices_connected_to_snap_vertex.count(edge.vertex_indices[!idx]))
					{
						edge.vertex_indices[idx] = drag_snap_vertex;
					}
				}
			}
			/// ...and remove the dragged vertex. This will kill any remaining connections
			RemoveVertex(dragged_vertex, false);
		}
	}
	drag_snapped = false;
	// Send updated value
	C4ConsoleQtShape::StopDragging();
}

void C4ConsoleQtGraph::SetValue(const C4Value &val)
{
	if (store_as_proplist)
	{
		// Load from a proplist
		// Expected format for graph e.g.:
		// { Vertices = [{ X=123, Y=456, Color=0xff0000 }, { X=789, Y=753 }], Edges = [{ Vertices=[0, 1], Color=0xff00ff, LineWidth=2 }]
		C4PropList *valp = val.getPropList();
		if (valp)
		{
			SetVerticesValue(valp->GetPropertyArray(P_Vertices));
			SetEdgesValue(valp->GetPropertyArray(P_Edges));
		}
	}
	else
	{
		// Load from an array
		// Array with two elements (vertices and edges) for general graph.
		// Expected format for graph e.g.:
		// [ [ [123, 456, 0xff0000], [789, 753] ], [ [0, 1, 0xff00ff, 2] ] ]
		C4ValueArray *arr = val.getArray();
		if (arr && arr->GetSize() == 2)
		{
			SetVerticesValue(arr->GetItem(0).getArray());
			SetEdgesValue(arr->GetItem(1).getArray());
		}
	}
}

void C4ConsoleQtGraph::SetVerticesValue(const C4ValueArray *vvertices)
{
	vertices.clear();
	if (vvertices)
	{
		vertices.reserve(vvertices->GetSize());
		for (int32_t i = 0; i < vvertices->GetSize(); ++i)
		{
			Vertex vtx;
			if (store_as_proplist)
			{
				C4PropList *vvertex = vvertices->GetItem(i).getPropList();
				if (!vvertex) continue;
				vtx.x = vvertex->GetPropertyInt(P_X);
				vtx.y = vvertex->GetPropertyInt(P_Y);
				if (vvertex->HasProperty(&::Strings.P[P_Color])) vtx.color = vvertex->GetPropertyInt(P_Color) | 0xff000000;
			}
			else
			{
				C4ValueArray *vvertex = vvertices->GetItem(i).getArray();
				if (!vvertex || vvertex->GetSize() < 2) continue;
				vtx.x = vvertex->GetItem(0).getInt();
				vtx.y = vvertex->GetItem(1).getInt();
				if (vvertex->GetSize() >= 3) vtx.color = vvertex->GetItem(2).getInt() | 0xff000000;
			}
			vertices.push_back(vtx);
		}
	}
}

void C4ConsoleQtGraph::SetEdgesValue(const C4ValueArray *vedges)
{
	edges.clear();
	if (vedges)
	{
		edges.reserve(vedges->GetSize());
		for (int32_t i = 0; i < vedges->GetSize(); ++i)
		{
			C4ValueArray *vedgevertices = nullptr;
			C4PropList *vedgeprops = nullptr;
			if (store_as_proplist)
			{
				vedgeprops = vedges->GetItem(i).getPropList();
				if (vedgeprops)
				{
					vedgevertices = vedgeprops->GetPropertyArray(P_Vertices);
				}
			}
			else
			{
				vedgevertices = vedges->GetItem(i).getArray();
			}
			if (vedgevertices && vedgevertices->GetSize() >= 2)
			{
				Edge edge;
				edge.vertex_indices[0] = vedgevertices->GetItem(0).getInt();
				edge.vertex_indices[1] = vedgevertices->GetItem(1).getInt();
				// Ignore invalid edge definitions
				if (edge.vertex_indices[0] < 0 || edge.vertex_indices[1] < 0) continue;
				if (edge.vertex_indices[0] >= vertices.size() || edge.vertex_indices[1] >= vertices.size()) continue;
				// Optional properties
				if (store_as_proplist)
				{
					if (vedgeprops->HasProperty(&::Strings.P[P_Color])) edge.color = vedgeprops->GetPropertyInt(P_Color) | 0xff000000;
					edge.line_thickness = vedgeprops->GetPropertyInt(P_LineWidth, edge.line_thickness);
				}
				else
				{
					if (vedgevertices->GetSize() >= 3) edge.color = vedgevertices->GetItem(2).getInt() | 0xff000000;
					if (vedgevertices->GetSize() >= 4) edge.line_thickness = vedgevertices->GetItem(3).getInt();
				}
				edges.push_back(edge);
			}
		}
	}
}

C4Value C4ConsoleQtGraph::GetValue() const
{
	// Store graph as nested arrays / proplists
	C4ValueArray *vvertices = GetVerticesValue();
	C4ValueArray *vedges = GetEdgesValue();
	if (store_as_proplist)
	{
		C4PropList *vmain = C4PropList::New();
		vmain->SetProperty(P_Vertices, C4VArray(vvertices));
		vmain->SetProperty(P_Edges, C4VArray(vedges));
		return C4VPropList(vmain);
	}
	else
	{
		C4ValueArray *vmain = new C4ValueArray(2);
		vmain->SetItem(0, C4VArray(vvertices));
		vmain->SetItem(1, C4VArray(vedges));
		return C4VArray(vmain);
	}
}

C4ValueArray *C4ConsoleQtGraph::GetVerticesValue() const
{
	// Store vertices
	C4ValueArray *vvertices = new C4ValueArray();
	vvertices->SetSize(vertices.size());
	int32_t i = 0;
	for (const Vertex &vtx : vertices)
	{
		C4Value valvtx;
		if (store_as_proplist)
		{
			C4PropList *vvtx = C4PropList::New();
			vvtx->SetProperty(P_X, C4VInt(vtx.x));
			vvtx->SetProperty(P_Y, C4VInt(vtx.y));
			if (vtx.color) vvtx->SetProperty(P_Color, C4VInt(vtx.color & 0xffffff));
			valvtx = C4VPropList(vvtx);
		}
		else
		{
			C4ValueArray *vvtx = new C4ValueArray(2);
			vvtx->SetItem(0, C4VInt(vtx.x));
			vvtx->SetItem(1, C4VInt(vtx.y));
			if (vtx.color) vvtx->SetItem(2, C4VInt(vtx.color & 0xffffff));
			valvtx = C4VArray(vvtx);
		}
		vvertices->SetItem(i++, valvtx);
	}
	return vvertices;
}

C4ValueArray *C4ConsoleQtGraph::GetEdgesValue() const
{
	C4ValueArray *vedges = new C4ValueArray();
	vedges->SetSize(edges.size());
	int32_t i = 0;
	for (const Edge &edge : edges)
	{
		C4Value valedge;
		C4ValueArray *vedge = new C4ValueArray(2);
		vedge->SetItem(0, C4VInt(edge.vertex_indices[0]));
		vedge->SetItem(1, C4VInt(edge.vertex_indices[1]));
		if (store_as_proplist)
		{
			C4PropList *vedgeprops = C4PropList::New();
			vedgeprops->SetProperty(P_Vertices, C4VArray(vedge));
			if (edge.color) vedgeprops->SetProperty(P_Color, C4VInt(edge.color & 0xffffff));
			if (edge.line_thickness != 1) vedgeprops->SetProperty(P_LineWidth, C4VInt(edge.line_thickness));
			valedge = C4VPropList(vedgeprops);
		}
		else
		{
			if (edge.color || edge.line_thickness != 1)
			{
				vedge->SetItem(2, C4VInt(edge.color & 0xffffff));
				if (edge.line_thickness != 1)
				{
					vedge->SetItem(3, C4VInt(edge.line_thickness));
				}
			}
			valedge = C4VArray(vedge);
		}
		vedges->SetItem(i++, valedge);
	}
	return vedges;
}

int32_t C4ConsoleQtGraph::InsertVertexOnEdge(int32_t split_edge_index, int32_t x, int32_t y)
{
	assert(split_edge_index >= 0 && split_edge_index < edges.size());
	// Insert vertex by splitting an edge
	Vertex new_vertex;
	int32_t new_vertex_index = vertices.size();
	new_vertex.x = x;
	new_vertex.y = y;
	vertices.push_back(new_vertex);
	Edge &split_edge = edges[split_edge_index];
	Edge new_edge = split_edge;
	new_edge.vertex_indices[0] = new_vertex_index;
	split_edge.vertex_indices[1] = new_vertex_index;
	edges.push_back(new_edge);
	// Return index of newly added vertex
	return new_vertex_index;
}

int32_t C4ConsoleQtGraph::InsertVertexOnVertex(int32_t target_vertex_index, int32_t x, int32_t y)
{
	assert(target_vertex_index >= 0 && target_vertex_index < vertices.size());
	// Insert vertex
	Vertex new_vertex;
	int32_t new_vertex_index = vertices.size();
	new_vertex.x = x;
	new_vertex.y = y;
	vertices.push_back(new_vertex);
	// Connect new vertex to target vertex
	Edge new_edge;
	new_edge.vertex_indices[0] = target_vertex_index;
	new_edge.vertex_indices[1] = new_vertex_index;
	edges.push_back(new_edge);
	// Return index of newly added vertex
	return new_vertex_index;
}

void C4ConsoleQtGraph::RemoveEdge(int32_t edge_index)
{
	assert(edge_index >= 0 && edge_index < edges.size());
	// Kill the edge
	Edge edge = edges[edge_index];
	edges.erase(edges.begin() + edge_index);
	// Kill unconnected vertices (but always keep at least one vertex)
	if (!GetEdgeCountForVertex(edge.vertex_indices[0]))
	{
		RemoveVertex(edge.vertex_indices[0], false);
		if (edge.vertex_indices[1] > edge.vertex_indices[0]) --edge.vertex_indices[1];
	}
	if (!GetEdgeCountForVertex(edge.vertex_indices[1]) && vertices.size() > 1) RemoveVertex(edge.vertex_indices[1], false);
}

void C4ConsoleQtGraph::RemoveVertex(int32_t remove_vertex_index, bool create_skip_connection)
{
	assert(remove_vertex_index >= 0 && remove_vertex_index < vertices.size() && vertices.size() >= 2);
	// (Assuming that edge lists are small. To keep the code simple, we're iterating over the whole edge list four times here)
	// If the vertex was connected to exactly two edges, fuse them into one
	if (create_skip_connection && GetEdgeCountForVertex(remove_vertex_index) == 2)
	{
		Edge *combine_edge = nullptr;
		for (Edge &edge : edges)
		{
			if (edge.connects_to(remove_vertex_index))
			{
				if (!combine_edge)
				{
					combine_edge = &edge;
				}
				else
				{
					// Let edge bridge over the removed vertex
					int32_t v = combine_edge->vertex_indices[combine_edge->vertex_indices[0] == remove_vertex_index];
					edge.vertex_indices[edge.vertex_indices[1] == remove_vertex_index] = v;
					// The removal check will remove the other edge (combine_edge)
					break;
				}
			}
		}
	}
	// Remove all edges involving this vertex
	auto rm_check = [remove_vertex_index](const Edge &edge) { return edge.connects_to(remove_vertex_index); };
	edges.erase(std::remove_if(edges.begin(), edges.end(), rm_check), edges.end());
	// Remove the vertex itself
	vertices.erase(vertices.begin() + remove_vertex_index);
	// Because vertex indices changed, update all edges that pointed to higher indices
	for (Edge &edge : edges)
	{
		for (int32_t &vi : edge.vertex_indices)
		{
			if (vi > remove_vertex_index)
			{
				--vi;
			}
		}
	}
}

int32_t C4ConsoleQtGraph::GetEdgeCountForVertex(int32_t vertex_index) const
{
	// Count all edges that connect to the given vertex
	auto count_check = [vertex_index](const Edge &edge) { return edge.connects_to(vertex_index); };
	return std::count_if(edges.begin(), edges.end(), count_check);
}

void C4ConsoleQtGraph::InsertVertexBefore(int32_t insert_vertex_index, int32_t x, int32_t y)
{
	// Insert vertex at position in vertex list
	Vertex new_vertex;
	new_vertex.x = x;
	new_vertex.y = y;
	vertices.insert(vertices.begin() + insert_vertex_index, new_vertex);
	// Update all edges pointing to vertices after this one
	for (Edge &edge : edges)
	{
		for (int32_t &vertex_index : edge.vertex_indices)
		{
			if (vertex_index >= insert_vertex_index)
			{
				++vertex_index;
			}
		}
	}
}

void C4ConsoleQtGraph::InsertEdgeBefore(int32_t insert_edge_index, int32_t vertex1, int32_t vertex2)
{
	// Insert edge at position in edge list
	Edge new_edge;
	new_edge.vertex_indices[0] = vertex1;
	new_edge.vertex_indices[1] = vertex2;
	edges.insert(edges.begin() + insert_edge_index, new_edge);
}


/* Open poly line */

void C4ConsoleQtPolyline::SetValue(const C4Value &val)
{
	// Set only vertices from value. Edges just connect all vertices.
	SetVerticesValue(val.getArray());
	edges.clear();
	if (vertices.size() >= 2)
	{
		edges.reserve(vertices.size());
		for (int32_t i = 0; i < vertices.size() - 1; ++i)
		{
			Edge edge;
			edge.vertex_indices[0] = i;
			edge.vertex_indices[1] = i + 1;
			edges.push_back(edge);
		}
	}
}

C4Value C4ConsoleQtPolyline::GetValue() const
{
	// Polyline: Only vertices; edges are implicit
	return C4VArray(GetVerticesValue());
}

int32_t C4ConsoleQtPolyline::InsertVertexOnEdge(int32_t split_edge_index, int32_t x, int32_t y)
{
	// Split an edge
	InsertVertexBefore(split_edge_index + 1, x, y);
	InsertEdgeBefore(split_edge_index, split_edge_index, split_edge_index + 1);
	edges[split_edge_index + 1].vertex_indices[0] = split_edge_index + 1;
	return split_edge_index + 1;
}

int32_t C4ConsoleQtPolyline::InsertVertexOnVertex(int32_t target_vertex_index, int32_t x, int32_t y)
{
	// Only allowed on first or last index: Extends the poly line
	if (!target_vertex_index)
	{
		InsertVertexBefore(0, x, y);
		InsertEdgeBefore(0, 0, 1);
		return target_vertex_index;
	}
	else if (target_vertex_index == vertices.size() - 1)
	{
		InsertVertexBefore(target_vertex_index + 1, x, y);
		InsertEdgeBefore(target_vertex_index, target_vertex_index, target_vertex_index + 1);
		return target_vertex_index + 1;
	}
	else
	{
		assert(false);
		return 0;
	}
}

void C4ConsoleQtPolyline::RemoveEdge(int32_t edge_index)
{
	// Find larger remaining side and keep it
	int32_t before_vertices = edge_index + 1;
	int32_t after_vertices = edges.size() - edge_index;
	if (before_vertices > after_vertices)
	{
		// Cut everything after removed edge
		for (int32_t i = 0; i < after_vertices; ++i)
		{
			RemoveVertex(vertices.size()-1, false);
		}
	}
	else
	{
		// Cut everything before removed edge
		for (int32_t i = 0; i < before_vertices; ++i) // O(n^2)
		{
			RemoveVertex(0, false);
		}
	}
}

bool C4ConsoleQtPolyline::IsVertexHit(int32_t vertex_index, Qt::CursorShape *drag_cursor, bool shift_down, bool ctrl_down)
{
	// Cannot add vertices from other vertices; only from edges
	// Except for end points where it would expand the poly line
	if (shift_down && !ctrl_down && vertex_index && vertex_index != vertices.size()-1)
	{
		return false;
	}
	return C4ConsoleQtGraph::IsVertexHit(vertex_index, drag_cursor, shift_down, ctrl_down);
}


/* Closed polygon */

void C4ConsoleQtPolygon::SetValue(const C4Value &val)
{
	// Set open polyline vertices and edges
	C4ConsoleQtPolyline::SetValue(val);
	// Add closing edge
	if (vertices.size() > 2)
	{
		Edge edge;
		edge.vertex_indices[0] = vertices.size() - 1;
		edge.vertex_indices[1] = 0;
		edges.push_back(edge);
	}
}

int32_t C4ConsoleQtPolygon::InsertVertexOnEdge(int32_t split_edge_index, int32_t x, int32_t y)
{
	// Split the edge
	int32_t rval = C4ConsoleQtPolyline::InsertVertexOnEdge(split_edge_index, x, y);
	// Close the polygon if it just became a triangle
	if (vertices.size() == 3)
	{
		Edge edge;
		edge.vertex_indices[0] = 2;
		edge.vertex_indices[1] = 0;
		edges.push_back(edge);
	}
	return rval;
}

int32_t C4ConsoleQtPolygon::InsertVertexOnVertex(int32_t target_vertex_index, int32_t x, int32_t y)
{
	// Never called because IsHit should return false
	assert(false);
	return 0;
}

void C4ConsoleQtPolygon::RemoveEdge(int32_t edge_index)
{
	// Remove both connected vertices (unless it's the last one)
	Edge edge = edges[edge_index];
	int32_t vertex_index = edge.vertex_indices[1];
	RemoveVertex(vertex_index, true);
	if (vertices.size() > 1)
	{
		RemoveVertex(vertex_index ? edge.vertex_indices[0] : vertices.size() - 1, true);
	}
}

bool C4ConsoleQtPolygon::IsVertexHit(int32_t vertex_index, Qt::CursorShape *drag_cursor, bool shift_down, bool ctrl_down)
{
	// Cannot add vertices from other vertices; only from edges
	if (shift_down && !ctrl_down)
	{
		return false;
	}
	// Skip C4ConsoleQtPolyline::IsVertexHit; it doesn't do sensible extra checks
	return C4ConsoleQtGraph::IsVertexHit(vertex_index, drag_cursor, shift_down, ctrl_down);
}


/* Shape list */

C4ConsoleQtShape *C4ConsoleQtShapes::CreateShape(class C4Object *for_obj, C4PropList *props, const C4Value &val, const class C4PropertyDelegateShape *parent_delegate)
{
	C4String *type = props->GetPropertyStr(P_Type);
	if (!type) return nullptr;
	C4ConsoleQtShape *shape = nullptr;
	if (type->GetData() == "rect") shape = new C4ConsoleQtRect(for_obj, props, parent_delegate);
	else if (type->GetData() == "circle") shape = new C4ConsoleQtCircle(for_obj, props, parent_delegate);
	else if (type->GetData() == "point") shape = new C4ConsoleQtPoint(for_obj, props, parent_delegate);
	else if (type->GetData() == "graph") shape = new C4ConsoleQtGraph(for_obj, props, parent_delegate);
	else if (type->GetData() == "polyline") shape = new C4ConsoleQtPolyline(for_obj, props, parent_delegate);
	else if (type->GetData() == "polygon") shape = new C4ConsoleQtPolygon(for_obj, props, parent_delegate);
	shape->SetValue(val);
	return shape;
}

void C4ConsoleQtShapes::AddShape(C4ConsoleQtShape *shape)
{
	if (shape) shapes.emplace_back(shape);
}

void C4ConsoleQtShapes::RemoveShape(C4ConsoleQtShape *shape)
{
	// Remove from list and currently moving shape
	shapes.remove_if([shape](auto &it) { return it.get() == shape; });
	if (dragging_shape == shape) dragging_shape = nullptr;
}

void C4ConsoleQtShapes::ClearShapes()
{
	shapes.clear();
	dragging_shape = nullptr;
	drag_cursor = Qt::CursorShape::ArrowCursor;
}

void C4ConsoleQtShapes::Draw(C4TargetFacet &cgo)
{
	// Draw all shapes with at least 1px line width
	ZoomDataStackItem zdsi(cgo.X, cgo.Y, cgo.Zoom);
	float line_width = std::max<float>(1.0f, 1.0f / cgo.Zoom);
	for (auto &shape : shapes) shape->Draw(cgo, line_width);
}

bool C4ConsoleQtShapes::MouseDown(float x, float y, float hit_range, bool shift_down, bool ctrl_down)
{
	// Check for shape hit and start dragging if a shape is in hit range
	int32_t hit_range_int = std::max(int32_t(hit_range + 0.5f), 1); // Using integer hit ranges for now
	// Ensure no leftover other shape
	if (dragging_shape) MouseUp(x, y, shift_down, ctrl_down);
	int32_t drag_border=-1;
	for (auto &shape : shapes)
	{
		if (shape->IsHit(x, y, hit_range_int, &drag_cursor, &drag_border, shift_down, ctrl_down))
		{
			dragging_shape = shape.get();
			if (dragging_shape->StartDragging(drag_border, int32_t(x), int32_t(y), shift_down, ctrl_down))
			{
				drag_x = x;
				drag_y = y;
			}
			else
			{
				// No dragging (the click may have done something else with the shape)
				dragging_shape = nullptr;
			}
			return true;
		}
	}
	return false;
}

void C4ConsoleQtShapes::MouseMove(float x, float y, bool left_down, float hit_range, bool shift_down, bool ctrl_down)
{
	// Check for shape hit and start dragging if a shape is in hit range
	int32_t hit_range_int = std::max(int32_t(hit_range + 0.5f), 1); // Using integer hit ranges for now
	// mouse down move: Execute shape dragging (full pixels only)
	if (dragging_shape && left_down)
	{
		int32_t dx = int32_t(round(x - drag_x)),
		        dy = int32_t(round(y - drag_y));
		if (dx || dy)
		{
			drag_x += dx;
			drag_y += dy;
			dragging_shape->Drag(drag_x, drag_y, dx, dy, hit_range_int, &drag_cursor);
		}
	}
	else if (!left_down)
	{
		// Just moving around: Update cursor
		drag_cursor = Qt::CursorShape::ArrowCursor;
		int32_t ignored;
		for (auto &shape : shapes) if (shape->IsHit(x, y, hit_range_int, &drag_cursor, &ignored, shift_down, ctrl_down)) break;
	}
	else
	{
		// Regular move: Reset drag cursor
		drag_cursor = Qt::CursorShape::ArrowCursor;
	}
}

void C4ConsoleQtShapes::MouseUp(float x, float y, bool shift_down, bool ctrl_down)
{
	// Stop dragging
	if (dragging_shape)
	{
		dragging_shape->StopDragging();
		dragging_shape = nullptr;
		drag_cursor = Qt::CursorShape::ArrowCursor;
	}
}


/* Shape pointer holder class */

void C4ConsoleQtShapeHolder::Clear()
{
	if (shape)
	{
		::Console.EditCursor.GetShapes()->RemoveShape(shape);
		shape = nullptr;
	}
}

void C4ConsoleQtShapeHolder::Set(C4ConsoleQtShape *new_shape)
{
	Clear();
	shape = new_shape;
	if (shape) ::Console.EditCursor.GetShapes()->AddShape(shape);
}
