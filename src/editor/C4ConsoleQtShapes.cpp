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
#include "editor/C4ConsoleQtPropListViewer.h" // for C4PropertyPath

/* Generic shape */

C4ConsoleQtShape::C4ConsoleQtShape(C4Object *for_obj, C4PropList *props, const class C4PropertyDelegateShape *parent_delegate, class C4ConsoleQtShapes *shape_list)
	: is_relative(true), dragging_border(-1), selected_border(-1), border_color(0xffff0000), parent_delegate(parent_delegate), shape_list(shape_list)
{
	rel_obj.SetPropList(for_obj);
	if (props)
	{
		is_relative = props->GetPropertyBool(P_Relative, is_relative);
		border_color = props->GetPropertyInt(P_Color) | 0xff000000;
		properties = C4VPropList(props);
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

bool C4ConsoleQtShape::Select(int32_t border)
{
	selected_border = border;
	emit BorderSelectionChanged();
	return true;
}

void C4ConsoleQtShape::ResetSelection()
{
	selected_border = -1;
	emit BorderSelectionChanged();
}


/* Rectangular shape*/

C4ConsoleQtRect::C4ConsoleQtRect(C4Object *for_obj, C4PropList *props, const class C4PropertyDelegateShape *parent_delegate, class C4ConsoleQtShapes *shape_list)
	: C4ConsoleQtShape(for_obj, props, parent_delegate, shape_list), left(0), top(0), right(10), bottom(10), store_as_proplist(false), properties_lowercase(false)
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

C4ConsoleQtCircle::C4ConsoleQtCircle(class C4Object *for_obj, C4PropList *props, const class C4PropertyDelegateShape *parent_delegate, class C4ConsoleQtShapes *shape_list)
	: C4ConsoleQtShape(for_obj, props, parent_delegate, shape_list), radius(10), cx(0), cy(0), can_move_center(false)
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

C4ConsoleQtPoint::C4ConsoleQtPoint(class C4Object *for_obj, C4PropList *props, const class C4PropertyDelegateShape *parent_delegate, class C4ConsoleQtShapes *shape_list)
	: C4ConsoleQtShape(for_obj, props, parent_delegate, shape_list), cx(0), cy(0)
{
	if (props)
	{
		horizontal_fix = props->GetPropertyBool(P_HorizontalFix);
		vertical_fix = props->GetPropertyBool(P_VerticalFix);
	}
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
		if (horizontal_fix && vertical_fix) *drag_cursor = Qt::CursorShape::ForbiddenCursor;
		if (horizontal_fix && !vertical_fix) *drag_cursor = Qt::CursorShape::SizeVerCursor;
		if (!horizontal_fix && vertical_fix) *drag_cursor = Qt::CursorShape::SizeHorCursor;
		if (!horizontal_fix && !vertical_fix) *drag_cursor = Qt::CursorShape::SizeAllCursor;
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
	if (horizontal_fix && !vertical_fix)
	{
		pDraw->DrawLineDw(cgo.Surface, x, y - d, x, y + d, clr, line_width);
		pDraw->DrawLineDw(cgo.Surface, x - d / 2, y - d, x + d / 2, y - d, clr, line_width);
		pDraw->DrawLineDw(cgo.Surface, x - d / 2, y + d, x + d / 2, y + d, clr, line_width);
	}
	else if (!horizontal_fix && vertical_fix)
	{
		pDraw->DrawLineDw(cgo.Surface, x - d, y, x + d, y, clr, line_width);
		pDraw->DrawLineDw(cgo.Surface, x - d, y - d / 2, x - d, y + d / 2, clr, line_width);
		pDraw->DrawLineDw(cgo.Surface, x + d, y - d / 2, x + d, y + d / 2, clr, line_width);
	}
	else
	{
		pDraw->DrawLineDw(cgo.Surface, x - d, y - d, x + d, y + d, clr, line_width);
		if (!horizontal_fix)
		{
			pDraw->DrawLineDw(cgo.Surface, x - d, y + d, x + d, y - d, clr, line_width);
		}
		pDraw->DrawCircleDw(cgo.Surface, x, y, dc, clr, line_width);
	}
}

void C4ConsoleQtPoint::Drag(int32_t x, int32_t y, int32_t dx, int32_t dy, int32_t hit_range, Qt::CursorShape *drag_cursor)
{
	if (!horizontal_fix) cx += dx;
	if (!vertical_fix) cy += dy;
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


void C4ConsoleQtGraph::GraphData::SetVerticesValue(const C4ValueArray *vvertices)
{
	vertices.clear();
	if (vvertices)
	{
		vertices.reserve(vvertices->GetSize());
		for (int32_t i = 0; i < vvertices->GetSize(); ++i)
		{
			Vertex vtx;
			C4PropList *vvertex = vvertices->GetItem(i).getPropList();
			if (!vvertex) continue;
			vtx.x = vvertex->GetPropertyInt(P_X);
			vtx.y = vvertex->GetPropertyInt(P_Y);
			if (vvertex->HasProperty(&::Strings.P[P_Color])) vtx.color = vvertex->GetPropertyInt(P_Color) | 0xff000000;
			vertices.push_back(vtx);
		}
	}
}

void C4ConsoleQtGraph::GraphData::SetEdgesValue(const C4ValueArray *vedges)
{
	edges.clear();
	if (vedges)
	{
		edges.reserve(vedges->GetSize());
		for (int32_t i = 0; i < vedges->GetSize(); ++i)
		{
			C4ValueArray *vedgevertices = nullptr;
			C4PropList *vedgeprops = nullptr;
			vedgeprops = vedges->GetItem(i).getPropList();
			if (vedgeprops)
			{
				vedgevertices = vedgeprops->GetPropertyArray(P_Vertices);
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
				if (vedgeprops->HasProperty(&::Strings.P[P_Color])) edge.color = vedgeprops->GetPropertyInt(P_Color) | 0xff000000;
				edge.line_thickness = vedgeprops->GetPropertyInt(P_LineWidth, edge.line_thickness);
				edges.push_back(edge);
			}
		}
	}
}

C4ValueArray *C4ConsoleQtGraph::GraphData::GetVerticesValue() const
{
	// Store vertices
	C4ValueArray *vvertices = new C4ValueArray();
	vvertices->SetSize(vertices.size());
	int32_t i = 0;
	for (const Vertex &vtx : vertices)
	{
		C4Value valvtx;
		C4PropList *vvtx = C4PropList::New();
		vvtx->SetProperty(P_X, C4VInt(vtx.x));
		vvtx->SetProperty(P_Y, C4VInt(vtx.y));
		if (vtx.color) vvtx->SetProperty(P_Color, C4VInt(vtx.color & 0xffffff));
		vvertices->SetItem(i++, C4VPropList(vvtx));
	}
	return vvertices;
}

C4ValueArray *C4ConsoleQtGraph::GraphData::GetEdgesValue() const
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
		C4PropList *vedgeprops = C4PropList::New();
		vedgeprops->SetProperty(P_Vertices, C4VArray(vedge));
		if (edge.color) vedgeprops->SetProperty(P_Color, C4VInt(edge.color & 0xffffff));
		if (edge.line_thickness != 1) vedgeprops->SetProperty(P_LineWidth, C4VInt(edge.line_thickness));
		vedges->SetItem(i++, C4VPropList(vedgeprops));
	}
	return vedges;
}

void C4ConsoleQtGraph::GraphData::SetVertexPos(int32_t vertex_index, int32_t new_x, int32_t new_y)
{
	// Validity check
	if (vertex_index < 0 || vertex_index >= vertices.size()) return;
	// Do change
	vertices[vertex_index].x = new_x;
	vertices[vertex_index].y = new_y;
}

void C4ConsoleQtGraph::GraphData::EditEdge(int32_t edge_index, int32_t change_vertex_index, int32_t new_vertex_index)
{
	// Validity check
	if (edge_index < 0 || edge_index >= edges.size()) return;
	if (new_vertex_index < 0 || new_vertex_index >= vertices.size()) return;
	if (change_vertex_index < 0 || change_vertex_index > 1) return;
	// No loopback edges
	Edge &edge = edges[edge_index];
	int32_t other_vertex_index = edge.vertex_indices[!change_vertex_index];
	if (new_vertex_index == other_vertex_index) return;
	// Do not allow duplicates
	for (Edge &check_edge : edges)
	{
		if (&check_edge != &edge)
		{
			if (check_edge.connects_to(new_vertex_index) && check_edge.connects_to(other_vertex_index)) return;
		}
	}
	// Perform change
	edge.vertex_indices[change_vertex_index] = new_vertex_index;
}

void C4ConsoleQtGraph::GraphData::InsertEdgeBefore(int32_t insert_edge_index, int32_t vertex1, int32_t vertex2)
{
	// Validity check
	if (insert_edge_index < 0 || insert_edge_index > edges.size()) return;
	if (vertex1 < 0 || vertex1 >= vertices.size()) return;
	if (vertex2 < 0 || vertex2 >= vertices.size()) return;
	// Do not allow duplicates
	for (Edge &check_edge : edges)
	{
		if (check_edge.connects_to(vertex1) && check_edge.connects_to(vertex2)) return;
	}
	// Insert edge at position in edge list
	Edge new_edge;
	new_edge.vertex_indices[0] = vertex1;
	new_edge.vertex_indices[1] = vertex2;
	edges.insert(edges.begin() + insert_edge_index, new_edge);
}

void C4ConsoleQtGraph::GraphData::InsertVertexBefore(int32_t insert_vertex_index, int32_t x, int32_t y)
{
	// Validity check
	if (insert_vertex_index < 0 || insert_vertex_index > vertices.size()) return;
	// Insert new vertex at desired position
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

void C4ConsoleQtGraph::GraphData::RemoveEdge(int32_t edge_index)
{
	// Validity check
	if (edge_index < 0 || edge_index >= edges.size()) return;
	// Kill the edge
	edges.erase(edges.begin() + edge_index);
}

void C4ConsoleQtGraph::GraphData::RemoveVertex(int32_t remove_vertex_index)
{
	// Validity vheck
	if (remove_vertex_index < 0 || remove_vertex_index >= vertices.size()) return;
	// Always keep at least one vertex
	if (vertices.size() == 1) return;
	assert(remove_vertex_index >= 0 && remove_vertex_index < vertices.size() && vertices.size() >= 2);
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

int32_t C4ConsoleQtGraph::GraphData::GetEdgeCountForVertex(int32_t vertex_index) const
{
	// Count all edges that connect to the given vertex
	auto count_check = [vertex_index](const Edge &edge) { return edge.connects_to(vertex_index); };
	return std::count_if(edges.begin(), edges.end(), count_check);
}

void C4ConsoleQtGraph::GraphData::EditGraphValue_SetVertexPos(C4ValueArray *vvertices, int32_t vertex_index, int32_t new_x, int32_t new_y)
{
	// Validity check
	if (vertex_index < 0 || vertex_index >= vvertices->GetSize()) return;
	// Do change
	C4PropList *vvertex = vvertices->GetItem(vertex_index).getPropList();
	if (!vvertex || vvertex->IsFrozen()) return;
	vvertex->SetProperty(P_X, C4VInt(new_x));
	vvertex->SetProperty(P_Y, C4VInt(new_y));
}

void C4ConsoleQtGraph::GraphData::EditGraphValue_EditEdge(C4ValueArray *vvertices, C4ValueArray *vedges, int32_t edge_index, int32_t change_vertex_index, int32_t new_vertex_index)
{
	// Validity check
	if (edge_index < 0 || edge_index >= vedges->GetSize()) return;
	if (new_vertex_index < 0 || new_vertex_index >= vvertices->GetSize()) return;
	if (change_vertex_index < 0 || change_vertex_index > 1) return;
	C4PropList *edge = vedges->GetItem(edge_index).getPropList();
	if (!edge) return;
	C4ValueArray *edge_vertices = edge->GetPropertyArray(P_Vertices);
	if (!edge_vertices || edge_vertices->IsFrozen()) return;
	// No loopback edges
	int32_t other_vertex_index = edge_vertices->GetItem(!change_vertex_index).getInt();
	if (new_vertex_index == other_vertex_index) return;
	// Do not allow duplicates
	for (int32_t i = 0; i < vedges->GetSize(); ++i)
	{
		if (i != edge_index)
		{
			if (EditGraphValue_EdgeConnectsTo(vedges->GetItem(i).getPropList(), other_vertex_index) && EditGraphValue_EdgeConnectsTo(vedges->GetItem(i).getPropList(), new_vertex_index))
			{
				return;
			}
		}
	}
	// Perform change
	edge_vertices->SetItem(change_vertex_index, C4VInt(new_vertex_index));
}

void C4ConsoleQtGraph::GraphData::EditGraphValue_InsertEdgeBefore(C4ValueArray *vvertices, C4ValueArray *vedges, int32_t insert_edge_index, int32_t vertex1, int32_t vertex2)
{
	// Validity check
	if (insert_edge_index < 0 || insert_edge_index > vedges->GetSize()) return;
	if (vertex1 < 0 || vertex1 >= vvertices->GetSize()) return;
	// Do not allow duplicates
	for (int32_t i = 0; i < vedges->GetSize(); ++i)
	{
		if (EditGraphValue_EdgeConnectsTo(vedges->GetItem(i).getPropList(), vertex1) && EditGraphValue_EdgeConnectsTo(vedges->GetItem(i).getPropList(), vertex2))
		{
			return;
		}
	}
	// Construct new edge
	C4PropList *new_edge = C4PropList::New();
	C4ValueArray *new_edge_indices = new C4ValueArray(2);
	new_edge_indices->SetItem(0, C4VInt(vertex1));
	new_edge_indices->SetItem(1, C4VInt(vertex2));
	new_edge->SetProperty(P_Vertices, C4VArray(new_edge_indices));
	// Insert edge at position in edge list by moving other edges up
	vedges->SetSize(vedges->GetSize() + 1);
	for (int32_t i = vedges->GetSize() - 1; i > insert_edge_index; --i)
	{
		vedges->SetItem(i, vedges->GetItem(i - 1));
	}
	vedges->SetItem(insert_edge_index, C4VPropList(new_edge));
}

void C4ConsoleQtGraph::GraphData::EditGraphValue_InsertVertexBefore(C4ValueArray *vvertices, C4ValueArray *vedges, int32_t insert_vertex_index, int32_t x, int32_t y)
{
	// Validity check
	if (insert_vertex_index < 0 || insert_vertex_index > vvertices->GetSize()) return;
	// Construct new vertex
	C4PropList *new_vertex = C4PropList::New();
	new_vertex->SetProperty(P_X, C4VInt(x));
	new_vertex->SetProperty(P_Y, C4VInt(y));
	// Insert new vertex at desired position by moving other vertices up
	vvertices->SetSize(vvertices->GetSize() + 1);
	for (int32_t i = vvertices->GetSize() - 1; i > insert_vertex_index; --i)
	{
		vvertices->SetItem(i, vvertices->GetItem(i - 1));
	}
	vvertices->SetItem(insert_vertex_index, C4VPropList(new_vertex));
	// Update all edges pointing to vertices after this one
	if (vedges)
	{
		for (int32_t i = 0; i < vedges->GetSize(); ++i)
		{
			C4PropList *edge = vedges->GetItem(i).getPropList();
			if (edge)
			{
				C4ValueArray *edge_vertices = edge->GetPropertyArray(P_Vertices);
				if (edge_vertices && edge_vertices->GetSize() >= 2)
				{
					for (int32_t j = 0; j < 2; ++j)
					{
						int32_t vertex_index = edge_vertices->GetItem(j).getInt();
						if (vertex_index >= insert_vertex_index)
						{
							edge_vertices->SetItem(j, C4VInt(vertex_index + 1));
						}
					}
				}
			}
		}
	}
}

void C4ConsoleQtGraph::GraphData::EditGraphValue_RemoveEdge(C4ValueArray *vvertices, C4ValueArray *vedges, int32_t edge_index)
{
	// Validity check
	if (edge_index < 0 || edge_index >= vedges->GetSize()) return;
	// Kill the edge
	for (int32_t i = edge_index; i < vedges->GetSize() - 1; ++i)
	{
		vedges->SetItem(i, vedges->GetItem(i + 1));
	}
	vedges->SetSize(vedges->GetSize() - 1);
}

void C4ConsoleQtGraph::GraphData::EditGraphValue_RemoveVertex(C4ValueArray *vvertices, C4ValueArray *vedges, int32_t remove_vertex_index)
{
	// Validity vheck
	if (remove_vertex_index < 0 || remove_vertex_index >= vvertices->GetSize()) return;
	// Always keep at least one vertex
	if (vvertices->GetSize() == 1) return;
	// Remove the vertex itself
	for (int32_t i = remove_vertex_index; i < vvertices->GetSize() - 1; ++i)
	{
		vvertices->SetItem(i, vvertices->GetItem(i + 1));
	}
	vvertices->SetSize(vvertices->GetSize() - 1);
	// Edge updates
	if (vedges)
	{
		for (int32_t edge_index = vedges->GetSize(); edge_index >= 0; --edge_index)
		{
			// Remove all edges involving this vertex and update vertices at higher index because they have been moved down
			C4PropList *edge = vedges->GetItem(edge_index).getPropList();
			if (edge)
			{
				C4ValueArray *edge_vertices = edge->GetPropertyArray(P_Vertices);
				if (edge_vertices && edge_vertices->GetSize() >= 2)
				{
					for (int32_t j = 0; j < 2; ++j)
					{
						int32_t v = edge_vertices->GetItem(j).getInt();
						if (v == remove_vertex_index)
						{
							EditGraphValue_RemoveEdge(vvertices, vedges, edge_index);
							break;
						}
						else if (v > remove_vertex_index)
						{
							edge_vertices->SetItem(j, C4VInt(v - 1));
						}
					}
				}
			}
		}
	}
}

bool C4ConsoleQtGraph::GraphData::EditGraphValue_EdgeConnectsTo(C4PropList *edge, int32_t vertex_index)
{
	// Check if either side of the edge connects to given vertex
	if (!edge) return false;
	C4ValueArray *edge_vertices = edge->GetPropertyArray(P_Vertices);
	if (!edge_vertices || edge_vertices->GetSize() < 2) return false;
	return edge_vertices->GetItem(0).getInt() == vertex_index || edge_vertices->GetItem(1).getInt() == vertex_index;
}


C4ConsoleQtGraph::C4ConsoleQtGraph(C4Object *for_obj, C4PropList *props, const class C4PropertyDelegateShape *parent_delegate, class C4ConsoleQtShapes *shape_list)
	: C4ConsoleQtShape(for_obj, props, parent_delegate, shape_list)
{
	// Def props
	if (props)
	{
		props->GetProperty(P_VertexDelegate, &vertex_delegate);
		props->GetProperty(P_EdgeDelegate, &edge_delegate);
		allow_vertex_selection = !!vertex_delegate.getPropList();
		allow_edge_selection = !!edge_delegate.getPropList();
		horizontal_fix = props->GetPropertyBool(P_HorizontalFix);
		vertical_fix = props->GetPropertyBool(P_VerticalFix);
		structure_fix = props->GetPropertyBool(P_StructureFix);
		draw_arrows = props->GetPropertyBool(P_DrawArrows, draw_arrows);
	}
}

bool C4ConsoleQtGraph::IsHit(int32_t x, int32_t y, int32_t hit_range, Qt::CursorShape *drag_cursor, int32_t *drag_border, bool shift_down, bool ctrl_down)
{
	// Check hit on vertices
	int32_t i = 0, best_hit_range = hit_range*hit_range * 6*6;
	bool has_hit = false;
	for (const Vertex &vtx : graph.vertices)
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
		for (const Edge &edge : graph.edges)
		{
			// Get affected vertices
			assert(edge.vertex_indices[0] >= 0 && edge.vertex_indices[1] >= 0);
			assert(edge.vertex_indices[0] < graph.vertices.size() && edge.vertex_indices[1] < graph.vertices.size());
			const Vertex &v0 = graph.vertices[edge.vertex_indices[0]];
			const Vertex &v1 = graph.vertices[edge.vertex_indices[1]];
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
	// Check if structure change is allowed
	if (has_hit && (shift_down || ctrl_down) && structure_fix)
	{
		*drag_cursor = Qt::CursorShape::ForbiddenCursor;
	}
	else if (*drag_cursor == Qt::CursorShape::SizeAllCursor)
	{
		// Adjust cursor for constrained movement
		if (horizontal_fix && vertical_fix)
		{
			*drag_cursor = Qt::CursorShape::PointingHandCursor;
		}
		else if (horizontal_fix)
		{
			*drag_cursor = Qt::CursorShape::SizeVerCursor;
		}
		else if (vertical_fix)
		{
			*drag_cursor = Qt::CursorShape::SizeHorCursor;
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
		if (graph.vertices.size() == 1)
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

void C4ConsoleQtGraph::DrawEdge(class C4TargetFacet &cgo, const Vertex &v0, const Vertex &v1, uint32_t clr, float line_width, float edge_width, bool highlight)
{
	float vx0 = AbsX(v0.x) + cgo.X - cgo.TargetX;
	float vy0 = AbsY(v0.y) + cgo.Y - cgo.TargetY;
	float vx1 = AbsX(v1.x) + cgo.X - cgo.TargetX;
	float vy1 = AbsY(v1.y) + cgo.Y - cgo.TargetY;
	float dx = v1.x - v0.x, dy = v1.y - v0.y;
	float d = sqrt(dx*dx + dy*dy);
	if (highlight)
	{
		// Selected edge is surrounded by a highlight
		float ddx = dy / d * edge_width;
		float ddy = dx / d * edge_width;
		pDraw->DrawLineDw(cgo.Surface, vx0 + ddx, vy0 + ddy, vx1 + ddx, vy1 + ddy, 0xffffffff, edge_width);
		pDraw->DrawLineDw(cgo.Surface, vx0 - ddx, vy0 - ddy, vx1 - ddx, vy1 - ddy, 0xffffffff, edge_width);
	}
	// Regular line draw
	pDraw->DrawLineDw(cgo.Surface, vx0, vy0, vx1, vy1, clr, edge_width);
	// Arrowheads
	if (draw_arrows)
	{
		// Arrowhead points to outer rim of vertex circle if possible
		float d_vtx = std::min<float>(d, line_width * 3 * sqrtf(2));
		float arrx = vx1 - dx / d * d_vtx;
		float arry = vy1 - dy / d * d_vtx;
		float arrowhead_size = std::max<float>(2.0, std::min<float>(d / 2, 8.0)) * edge_width;
		float ddx1 = (-dy / 2 - dx);
		float ddy1 = (+dx / 2 - dy);
		float d1 = sqrt(ddx1*ddx1 + ddy1*ddy1);
		float ddx2 = (+dy / 2 - dx);
		float ddy2 = (-dx / 2 - dy);
		float d2 = sqrt(ddx2*ddx2 + ddy2*ddy2);
		pDraw->DrawLineDw(cgo.Surface, arrx, arry, arrx + ddx1 * arrowhead_size / d1, arry + ddy1 * arrowhead_size / d1, clr, edge_width);
		pDraw->DrawLineDw(cgo.Surface, arrx, arry, arrx + ddx2 * arrowhead_size / d2, arry + ddy2 * arrowhead_size / d2, clr, edge_width);
	}
}

void C4ConsoleQtGraph::Draw(class C4TargetFacet &cgo, float line_width)
{
	// Draw edges as lines
	int32_t i = 0;
	for (const Edge &edge : graph.edges)
	{
		uint32_t clr = GetBorderColor(EdgeToDragBorder(i), false, edge.color);
		assert(edge.vertex_indices[0] >= 0 && edge.vertex_indices[1] >= 0);
		assert(edge.vertex_indices[0] < graph.vertices.size() && edge.vertex_indices[1] < graph.vertices.size());
		const Vertex &v0 = graph.vertices[edge.vertex_indices[0]];
		const Vertex &v1 = graph.vertices[edge.vertex_indices[1]];
		float edge_width = line_width * edge.line_thickness;
		bool highlight = (IsEdgeDrag(selected_border) && DragBorderToEdge(selected_border) == i);
		DrawEdge(cgo, v0, v1, clr, line_width, edge_width, highlight);
		++i;
	}
	// Draw vertices as circles with cross inside
	i = 0;
	for (const Vertex &vtx : graph.vertices)
	{
		uint32_t clr = GetBorderColor(VertexToDragBorder(i), false, vtx.color);
		float d = line_width * 3;
		float dc = sqrtf(2) * d;
		int32_t x = AbsX(vtx.x) + cgo.X - cgo.TargetX;
		int32_t y = AbsY(vtx.y) + cgo.Y - cgo.TargetY;
		pDraw->DrawLineDw(cgo.Surface, x - d, y - d, x + d, y + d, clr, line_width);
		pDraw->DrawLineDw(cgo.Surface, x - d, y + d, x + d, y - d, clr, line_width);
		// Selected vertex is surrounded by a highlight
		if (IsVertexDrag(selected_border) && DragBorderToVertex(selected_border) == i)
		{
			pDraw->DrawCircleDw(cgo.Surface, x, y, dc + line_width, 0xffffffff, line_width);
			pDraw->DrawCircleDw(cgo.Surface, x, y, dc - line_width, 0xffffffff, line_width);
		}
		pDraw->DrawCircleDw(cgo.Surface, x, y, dc, clr, line_width);
		++i;
	}
}

bool C4ConsoleQtGraph::StartDragging(int32_t *border, int32_t x, int32_t y, bool shift_down, bool ctrl_down)
{
	assert(*border != -1);
	drag_snap_offset_x = drag_snap_offset_y = 0;
	drag_snapped = false;
	drag_snap_vertex = -1;
	drag_source_vertex_index = -1;
	if (shift_down && !ctrl_down && !structure_fix)
	{
		// Shift: Insert vertex
		if (IsEdgeDrag(*border))
		{
			// Insert on edge
			*border = dragging_border = VertexToDragBorder(InsertVertexOnEdge(DragBorderToEdge(*border), x - AbsX(), y - AbsY()));
		}
		else
		{
			// Insert from other vertex
			drag_source_vertex_index = DragBorderToVertex(*border);
			*border = dragging_border = VertexToDragBorder(InsertVertexOnVertex(drag_source_vertex_index, x - AbsX(), y - AbsY()));
		}
		// Start dragging
		return true;
	}
	else if (ctrl_down && !shift_down && !structure_fix)
	{
		// Ctrl: Delete vertex or edge
		if (IsEdgeDrag(*border))
		{
			// Delete edge
			RemoveEdge(DragBorderToEdge(*border));
		}
		else
		{
			// Remove vertex unless it is the last one
			// If there's only one vertex left, just keep it
			if (graph.vertices.size() > 1) RemoveVertex(DragBorderToVertex(*border), true);
		}
		// Notify script
		emit ShapeDragged();
		// Do not start dragging or selecting
		*border = -1;
		return false;
	}
	else if (!shift_down && !ctrl_down)
	{
		// Regular dragging of vertices
		if (IsVertexDrag(*border))
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
		// Unknown modifiers or trying to modify a structure-fixed graph
		return false;
	}
}

void C4ConsoleQtGraph::Drag(int32_t x, int32_t y, int32_t dx, int32_t dy, int32_t hit_range, Qt::CursorShape *drag_cursor)
{
	// Dragging: Only vertices
	if (IsVertexDrag(dragging_border) && DragBorderToVertex(dragging_border) < graph.vertices.size())
	{
		int32_t dragged_vertex_index = DragBorderToVertex(dragging_border);
		Vertex &dragged_vertex = graph.vertices[dragged_vertex_index];
		// Regular vertex movement
		dx -= drag_snap_offset_x;
		dy -= drag_snap_offset_y;
		if (!horizontal_fix) dragged_vertex.x += dx;
		if (!vertical_fix) dragged_vertex.y += dy;
		// Handle snap to combine with other vertices
		if (!IsPolyline() && !structure_fix)
		{
			int32_t i = 0;
			drag_snap_vertex = -1;
			int32_t best_hit_range_sq = hit_range * hit_range * 4;
			for (Vertex &check_vertex : graph.vertices)
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
				drag_snap_offset_x = graph.vertices[drag_snap_vertex].x - dragged_vertex.x;
				drag_snap_offset_y = graph.vertices[drag_snap_vertex].y - dragged_vertex.y;
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
		// Regular drag: Emit signal to update value
		EditGraph(true, C4ControlEditGraph::Action::CEG_SetVertexPos, dragged_vertex_index, dragged_vertex.x, dragged_vertex.y);
	}
}

void C4ConsoleQtGraph::StopDragging()
{
	// Is it a vertex recombination?
	if (IsVertexDrag(dragging_border))
	{
		int32_t dragged_vertex = DragBorderToVertex(dragging_border);
		if (drag_snapped)
		{
			if (dragged_vertex && dragged_vertex != drag_snap_vertex)
			{
				// dragged_vertex is to be merged into drag_snap_vertex (keeping drag_snap_vertex)
				// find all edge targets already existing for drag_snap_vertex (this may include dragged_vertex)
				std::set<int32_t> vertices_connected_to_snap_vertex;
				int32_t idx;
				for (Edge &edge : graph.edges)
				{
					if (edge.connects_to(drag_snap_vertex, &idx))
					{
						vertices_connected_to_snap_vertex.insert(edge.vertex_indices[!idx]);
					}
				}
				// make sure that any connection from dragged_vertex to drag_snap_vertex will be excluded by the check
				vertices_connected_to_snap_vertex.insert(drag_snap_vertex);
				// move all connections that did not exist yet from dragged_vertex to drag_snap_vertex
				int32_t i_edge = 0;
				for (Edge &edge : graph.edges)
				{
					if (edge.connects_to(dragged_vertex, &idx))
					{
						if (!vertices_connected_to_snap_vertex.count(edge.vertex_indices[!idx]))
						{
							EditEdge(i_edge, idx, drag_snap_vertex);
						}
					}
					++i_edge;
				}
				// ...and remove the dragged vertex. This will kill any remaining connections
				RemoveVertex(dragged_vertex, false);
				return;
			}
		}
		// Regular drag: Emit signal to update value
		EditGraph(true, C4ControlEditGraph::Action::CEG_SetVertexPos, dragged_vertex, graph.vertices[dragged_vertex].x, graph.vertices[dragged_vertex].y);
	}
	drag_snapped = false;
	// Reset drag
	C4ConsoleQtShape::StopDragging();
}

void C4ConsoleQtGraph::SetValue(const C4Value &val)
{
	// Load from a proplist
	// Expected format for graph e.g.:
	// { Vertices = [{ X=123, Y=456, Color=0xff0000 }, { X=789, Y=753 }], Edges = [{ Vertices=[0, 1], Color=0xff00ff, LineWidth=2 }]
	C4PropList *valp = val.getPropList();
	if (valp)
	{
		graph.SetVerticesValue(valp->GetPropertyArray(P_Vertices));
		graph.SetEdgesValue(valp->GetPropertyArray(P_Edges));
	}
}

C4Value C4ConsoleQtGraph::GetValue() const
{
	// Store graph as nested arrays / proplists
	C4ValueArray *vvertices = graph.GetVerticesValue();
	C4ValueArray *vedges = graph.GetEdgesValue();
	C4PropList *vmain = C4PropList::New();
	vmain->SetProperty(P_Vertices, C4VArray(vvertices));
	vmain->SetProperty(P_Edges, C4VArray(vedges));
	return C4VPropList(vmain);
}

void C4ConsoleQtGraph::EditEdge(int32_t edge_index, int32_t change_vertex_index, int32_t new_vertex)
{
	EditGraph(true, C4ControlEditGraph::Action::CEG_EditEdge, edge_index, change_vertex_index, new_vertex);
}

int32_t C4ConsoleQtGraph::AddVertex(int32_t new_x, int32_t new_y)
{
	EditGraph(true, C4ControlEditGraph::Action::CEG_InsertVertex, graph.vertices.size(), new_x, new_y);
	return graph.vertices.size() - 1;
}

int32_t C4ConsoleQtGraph::AddEdge(int32_t connect_vertex_index_1, int32_t connect_vertex_index_2)
{
	EditGraph(true, C4ControlEditGraph::Action::CEG_InsertEdge, graph.edges.size(), connect_vertex_index_1, connect_vertex_index_2);
	return graph.edges.size() - 1;
}

int32_t C4ConsoleQtGraph::InsertVertexOnEdge(int32_t split_edge_index, int32_t x, int32_t y)
{
	assert(split_edge_index >= 0 && split_edge_index < graph.edges.size());
	// Insert vertex by splitting an edge
	int32_t new_vertex_index = AddVertex(x, y);
	AddEdge(new_vertex_index, graph.edges[split_edge_index].vertex_indices[1]);
	EditEdge(split_edge_index, 1, new_vertex_index);
	// Return index of newly added vertex
	return new_vertex_index;
}

int32_t C4ConsoleQtGraph::InsertVertexOnVertex(int32_t target_vertex_index, int32_t x, int32_t y)
{
	assert(target_vertex_index >= 0 && target_vertex_index < graph.vertices.size());
	// Insert vertex
	int32_t new_vertex_index = AddVertex(x, y);
	// Connect new vertex to target vertex
	AddEdge(target_vertex_index, new_vertex_index);
	// Return index of newly added vertex
	return new_vertex_index;
}

void C4ConsoleQtGraph::RemoveEdge(int32_t edge_index)
{
	assert(edge_index >= 0 && edge_index < graph.edges.size());
	// Remove the edge
	Edge removed_edge = graph.edges[edge_index];
	EditGraph(true, C4ControlEditGraph::Action::CEG_RemoveEdge, edge_index, 0, 0);
	// Kill unconnected vertices
	if (!graph.GetEdgeCountForVertex(removed_edge.vertex_indices[0]))
	{
		RemoveVertex(removed_edge.vertex_indices[0], false);
		// Removing the first vertex will have shifted the index of the second vertex
		if (removed_edge.vertex_indices[1] > removed_edge.vertex_indices[0]) --removed_edge.vertex_indices[1];
	}
	if (!graph.GetEdgeCountForVertex(removed_edge.vertex_indices[1]))
	{
		// (RemoveVertex will always keep at least one vertex)
		RemoveVertex(removed_edge.vertex_indices[1], false);
	}
}

void C4ConsoleQtGraph::RemoveVertex(int32_t remove_vertex_index, bool create_skip_connection)
{
	assert(remove_vertex_index >= 0 && remove_vertex_index < graph.vertices.size());
	// Create skip connection if the vertex had exactly two edges
	if (create_skip_connection && graph.GetEdgeCountForVertex(remove_vertex_index) == 2)
	{
		Edge *combine_edge = nullptr;
		int32_t i = 0;
		for (Edge &edge : graph.edges)
		{
			int32_t connect_idx;
			if (edge.connects_to(remove_vertex_index, &connect_idx))
			{
				if (!combine_edge)
				{
					combine_edge = &edge;
				}
				else
				{
					// Let edge bridge over the removed vertex
					int32_t v = combine_edge->vertex_indices[combine_edge->vertex_indices[0] == remove_vertex_index];
					EditEdge(i, connect_idx, v);
					// The removal check will remove the other edge (combine_edge)
					break;
				}
			}
			++i;
		}
	}
	// Remove the actual vertex
	EditGraph(true, C4ControlEditGraph::Action::CEG_RemoveVertex, remove_vertex_index, 0, 0);
}

void C4ConsoleQtGraph::InsertVertexBefore(int32_t insert_vertex_index, int32_t x, int32_t y)
{
	// Insert vertex at position in vertex list
	EditGraph(true, C4ControlEditGraph::Action::CEG_InsertVertex, insert_vertex_index, x, y);
}

void C4ConsoleQtGraph::InsertEdgeBefore(int32_t insert_edge_index, int32_t connect_vertex_index_1, int32_t connect_vertex_index_2)
{
	// Insert vertex at position in vertex list
	EditGraph(true, C4ControlEditGraph::Action::CEG_InsertEdge, insert_edge_index, connect_vertex_index_1, connect_vertex_index_2);
}

bool C4ConsoleQtGraph::IsSelectionAllowed(int32_t border) const
{
	// Independent selection settings for vertices and edges
	if (IsVertexDrag(border)) return allow_vertex_selection;
	if (IsEdgeDrag(border)) return allow_edge_selection;
	return false;
}

bool C4ConsoleQtGraph::GetSelectedData(const C4Value &shape_val, const class C4PropertyPath &shape_property_path, C4PropList **shape_item_editorprops, C4PropList **shape_item_value, C4String **shape_item_name, class C4PropertyPath *shape_item_target_path) const
{
	// Selection may either be a vertex or an edge
	C4PropList *delegate = nullptr;
	C4ValueArray *vitems = nullptr;
	int32_t selected_item = -1;
	C4PropertyPath item_array_path;
	if (IsVertexDrag(selected_border))
	{
		selected_item = DragBorderToVertex(selected_border);
		delegate = vertex_delegate.getPropList();
		C4PropList *shape_val_proplist = shape_val.getPropList();
		if (!shape_val_proplist) return false;
		vitems = shape_val_proplist->GetPropertyArray(P_Vertices);
		item_array_path = C4PropertyPath(shape_property_path, ::Strings.P[P_Vertices].GetCStr());
	}
	else if (IsEdgeDrag(selected_border))
	{
		selected_item = DragBorderToEdge(selected_border);
		delegate = edge_delegate.getPropList();
		C4PropList *shape_val_proplist = shape_val.getPropList();
		if (!shape_val_proplist) return false;
		vitems = shape_val_proplist->GetPropertyArray(P_Edges);
		item_array_path = C4PropertyPath(shape_property_path, ::Strings.P[P_Edges].GetCStr());
	}
	if (!delegate) return false;
	// Get edge/vertex value
	if (!vitems || vitems->GetSize() <= selected_item) return false;
	*shape_item_value = vitems->GetItem(selected_item).getPropList();
	if (!*shape_item_value) return false;
	// Get edge/vertex path
	*shape_item_target_path = C4PropertyPath(item_array_path, selected_item);
	// Get delegate information
	*shape_item_name = delegate->GetPropertyStr(P_Name);
	*shape_item_editorprops = delegate->GetPropertyPropList(P_EditorProps);
	if (!*shape_item_editorprops) return false; // required
	return true;
}

void C4ConsoleQtGraph::EditGraph(bool signal_change, C4ControlEditGraph::Action action, int32_t index, int32_t x, int32_t y)
{
	// Execute edit operation on graph
	// Validity checks are done by the individual operations
	switch (action)
	{
	case C4ControlEditGraph::Action::CEG_None:
		// Should never be sent
		assert(false);
		break;
	case C4ControlEditGraph::Action::CEG_SetVertexPos:
		graph.SetVertexPos(index, x, y);
		break;
	case C4ControlEditGraph::Action::CEG_EditEdge:
		graph.EditEdge(index, x, y);
		break;
	case C4ControlEditGraph::Action::CEG_InsertVertex:
		graph.InsertVertexBefore(index, x, y);
		break;
	case C4ControlEditGraph::Action::CEG_InsertEdge:
		graph.InsertEdgeBefore(index, x, y);
		break;
	case C4ControlEditGraph::Action::CEG_RemoveVertex:
		graph.RemoveVertex(index);
		break;
	case C4ControlEditGraph::Action::CEG_RemoveEdge:
		graph.RemoveEdge(index);
		break;
	}
	// Signal change to the owner of this graph
	// The owner should propagate the value through the control queue to edit the underlying script data and update any other clients
	if (signal_change)
	{
		emit GraphEdit(action, index, x, y);
	}
}

void C4ConsoleQtGraph::EditGraphValue(C4Value &val, C4ControlEditGraph::Action action, int32_t index, int32_t x, int32_t y)
{
	// Execute action on C4Value representing graph
	// Get graph props: Can be either vertices as array (for polyline/polygon) or proplist with Vertices and Edges props (for general graphs)
	C4ValueArray *vvertices = nullptr, *vedges = nullptr;
	C4PropList *vval = val.getPropList();
	if (vval)
	{
		if (vval->IsFrozen()) return;
		vvertices = vval->GetPropertyArray(P_Vertices);
		vedges = vval->GetPropertyArray(P_Edges);
		if (!vvertices || !vedges || vvertices->IsFrozen() || vedges->IsFrozen()) return;
	}
	else
	{
		vvertices = val.getArray();
		if (!vvertices || vvertices->IsFrozen()) return;
	}
	// Validity checks are done by the individual operations
	switch (action)
	{
	case C4ControlEditGraph::Action::CEG_None:
		// Should never be sent
		assert(false);
		break;
	case C4ControlEditGraph::Action::CEG_SetVertexPos:
	{
		GraphData::EditGraphValue_SetVertexPos(vvertices, index, x, y);
		break;
	}
	case C4ControlEditGraph::Action::CEG_EditEdge:
	{
		if (vedges) // Ignore on polyline / polygon
		{
			GraphData::EditGraphValue_EditEdge(vvertices, vedges, index, x, y);
		}
		break;
	}
	case C4ControlEditGraph::Action::CEG_InsertVertex:
	{
		GraphData::EditGraphValue_InsertVertexBefore(vvertices, vedges, index, x, y);
		break;
	}
	case C4ControlEditGraph::Action::CEG_InsertEdge:
	{
		if (vedges) // Ignore on polyline / polygon
		{
			GraphData::EditGraphValue_InsertEdgeBefore(vvertices, vedges, index, x, y);
		}
		break;
	}
	case C4ControlEditGraph::Action::CEG_RemoveVertex:
	{
		GraphData::EditGraphValue_RemoveVertex(vvertices, vedges, index);
		break;
	}
	case C4ControlEditGraph::Action::CEG_RemoveEdge:
	{
		if (vedges) // Ignore on polyline / polygon
		{
			GraphData::EditGraphValue_RemoveEdge(vvertices, vedges, index);
		}
		break;
	}
	}
}


/* Open poly line */

C4ConsoleQtPolyline::C4ConsoleQtPolyline(class C4Object *for_obj, C4PropList *props, const class C4PropertyDelegateShape *parent_delegate, class C4ConsoleQtShapes *shape_list)
	: C4ConsoleQtGraph(for_obj, props, parent_delegate, shape_list)
{
	if (props)
	{
		start_from_object = props->GetPropertyBool(P_StartFromObject, start_from_object);
	}
}

void C4ConsoleQtPolyline::Draw(class C4TargetFacet &cgo, float line_width)
{
	// Line from object center to first vertex
	if (start_from_object && graph.vertices.size())
	{
		C4ConsoleQtGraph::Vertex v0;
		if (!is_relative)
		{
			// In non-relative mode, the root coordinate needs to be pushed to the object manually
			C4Object *obj = rel_obj.getObj();
			if (obj)
			{
				v0.x += obj->GetX();
				v0.y += obj->GetY();
			}
		}
		DrawEdge(cgo, v0, graph.vertices[0], border_color, line_width, line_width, false);
	}
	// Remaining polyline is handled by regular graph drawing
	C4ConsoleQtGraph::Draw(cgo, line_width);
}

void C4ConsoleQtPolyline::SetValue(const C4Value &val)
{
	// Set only vertices from value. Edges just connect all vertices.
	graph.SetVerticesValue(val.getArray());
	// Init edges directly to avoid unnecessery checks done by insert edge
	graph.edges.clear();
	if (graph.vertices.size() >= 2)
	{
		graph.edges.reserve(graph.vertices.size());
		for (int32_t i = 0; i < graph.vertices.size() - 1; ++i)
		{
			Edge edge;
			edge.vertex_indices[0] = i;
			edge.vertex_indices[1] = i + 1;
			graph.edges.push_back(edge);
		}
	}
}

C4Value C4ConsoleQtPolyline::GetValue() const
{
	// Polyline: Only vertices; edges are implicit
	return C4VArray(graph.GetVerticesValue());
}

int32_t C4ConsoleQtPolyline::InsertVertexOnEdge(int32_t split_edge_index, int32_t x, int32_t y)
{
	// Split an edge
	InsertVertexBefore(split_edge_index + 1, x, y);
	InsertEdgeBefore(split_edge_index, split_edge_index, split_edge_index + 1);
	EditEdge(split_edge_index + 1, 0, split_edge_index + 1);
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
	else if (target_vertex_index == graph.vertices.size() - 1)
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
	int32_t after_vertices = graph.edges.size() - edge_index;
	if (before_vertices > after_vertices)
	{
		// Cut everything after removed edge
		for (int32_t i = 0; i < after_vertices; ++i)
		{
			RemoveVertex(graph.vertices.size()-1, false);
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
	if (shift_down && !ctrl_down && vertex_index && vertex_index != graph.vertices.size()-1)
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
	if (graph.vertices.size() > 2)
	{
		Edge edge;
		edge.vertex_indices[0] = graph.vertices.size() - 1;
		edge.vertex_indices[1] = 0;
		graph.edges.push_back(edge);
	}
}

int32_t C4ConsoleQtPolygon::InsertVertexOnEdge(int32_t split_edge_index, int32_t x, int32_t y)
{
	// Split the edge
	int32_t rval = C4ConsoleQtPolyline::InsertVertexOnEdge(split_edge_index, x, y);
	// Close the polygon if it just became a triangle
	if (graph.vertices.size() == 3)
	{
		AddEdge(2, 0);
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
	Edge edge = graph.edges[edge_index];
	int32_t vertex_index = edge.vertex_indices[1];
	RemoveVertex(vertex_index, true);
	if (graph.vertices.size() > 1)
	{
		RemoveVertex(vertex_index ? edge.vertex_indices[0] : graph.vertices.size() - 1, true);
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
	if (type->GetData() == "rect") shape = new C4ConsoleQtRect(for_obj, props, parent_delegate, this);
	else if (type->GetData() == "circle") shape = new C4ConsoleQtCircle(for_obj, props, parent_delegate, this);
	else if (type->GetData() == "point") shape = new C4ConsoleQtPoint(for_obj, props, parent_delegate, this);
	else if (type->GetData() == "graph") shape = new C4ConsoleQtGraph(for_obj, props, parent_delegate, this);
	else if (type->GetData() == "polyline") shape = new C4ConsoleQtPolyline(for_obj, props, parent_delegate, this);
	else if (type->GetData() == "polygon") shape = new C4ConsoleQtPolygon(for_obj, props, parent_delegate, this);
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
	if (selected_shape == shape) selected_shape = nullptr;
}

void C4ConsoleQtShapes::ClearShapes()
{
	shapes.clear();
	dragging_shape = selected_shape = nullptr;
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
			if (dragging_shape->StartDragging(&drag_border, int32_t(x), int32_t(y), shift_down, ctrl_down))
			{
				drag_x = x;
				drag_y = y;
			}
			else
			{
				// No dragging (the click may have done something else with the shape)
				dragging_shape = nullptr;
			}
			// Selection (independent of dragging; but drag may have changed the border)
			if (drag_border != -1)
			{
				if (shape->IsSelectionAllowed(drag_border))
				{
					SetSelectedShape(&*shape, drag_border);
				}
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

void C4ConsoleQtShapes::SetSelectedShape(C4ConsoleQtShape *new_selection, int32_t selected_border)
{
	// Deselect old and select new
	if (selected_shape) selected_shape->ResetSelection();
	selected_shape = new_selection;
	if (selected_shape)
	{
		if (!selected_shape->Select(selected_border))
		{
			// Selection failure? Deselect if still selected.
			if (selected_shape == new_selection) selected_shape = nullptr;
		}
	}
}

bool C4ConsoleQtShapes::GetSelectedShapeData(const C4Value &shape_val, const class C4PropertyPath &shape_property_path, C4PropList **shape_item_editorprops, C4PropList **shape_item_value, C4String **shape_item_name, class C4PropertyPath *shape_item_target_path) const
{
	if (!selected_shape) return false;
	return selected_shape->GetSelectedData(shape_val, shape_property_path, shape_item_editorprops, shape_item_value, shape_item_name, shape_item_target_path);
}


/* Shape pointer holder class */

bool C4ConsoleQtShapeHolder::last_visit_flag = false;

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
	if (shape == new_shape) return;
	Clear();
	shape = new_shape;
	if (shape) ::Console.EditCursor.GetShapes()->AddShape(shape);
}
