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
#include "editor/C4Console.h"
#include "editor/C4ConsoleQtState.h"
#include "editor/C4ConsoleQtShapes.h"
#include "graphics/C4FacetEx.h"
#include "object/C4Object.h"

/* Generic shape */

C4ConsoleQtShape::C4ConsoleQtShape(C4Object *for_obj, C4PropList *props)
	: is_relative(false), dragging_border(-1), border_color(0xffff0000)
{
	rel_obj.SetPropList(for_obj);
	if (props)
	{
		is_relative = props->GetPropertyBool(P_Relative);
		border_color = props->GetPropertyInt(P_Color) | 0xff000000;
	}
}

uint32_t C4ConsoleQtShape::GetBorderColor(int32_t border_index, bool dragging_border_is_bitmask) const
{
	// Return shape color, or dragged border color if index is the border currently being dragged
	if (IsDragging())
		if ((dragging_border == border_index) || (dragging_border_is_bitmask && (dragging_border & border_index)))
			return 0xffffffff;
	return border_color;
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


/* Rectangular shape*/

C4ConsoleQtRect::C4ConsoleQtRect(C4Object *for_obj, C4PropList *props, const C4Value &val)
	: C4ConsoleQtShape(for_obj, props), left(0), top(0), right(10), bottom(10)
{
	// Expect rect to be given as [left,top,width,height]
	C4ValueArray *varr = val.getArray();
	if (varr && varr->GetSize() >= 4)
	{
		left = varr->GetItem(0).getInt();
		top = varr->GetItem(1).getInt();
		right = left + varr->GetItem(2).getInt()-1; // right/bottom borders are drawn inclusively
		bottom = top + varr->GetItem(3).getInt()-1;
	}
}

bool C4ConsoleQtRect::IsHit(int32_t x, int32_t y, int32_t hit_range, Qt::CursorShape *drag_cursor, int32_t *drag_border)
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

void C4ConsoleQtRect::Drag(int32_t x, int32_t y, int32_t dx, int32_t dy)
{
	if (dragging_border & CNAT_Left) left += dx;
	if (dragging_border & CNAT_Top) top += dy;
	if (dragging_border & CNAT_Right) right += dx;
	if (dragging_border & CNAT_Bottom) bottom += dy;
	if (left > right) std::swap(left, right);
	if (top > bottom) std::swap(top, bottom);
}

C4Value C4ConsoleQtRect::GetValue() const
{
	// Return array: Convert left/top/right/bottom (inclusive) to left/top/width/height
	C4ValueArray *pos_array = new C4ValueArray(4);
	pos_array->SetItem(0, C4VInt(left));
	pos_array->SetItem(1, C4VInt(top));
	pos_array->SetItem(2, C4VInt(right - left + 1));
	pos_array->SetItem(3, C4VInt(bottom - top + 1));
	return C4VArray(pos_array);
}


/* Circle shape */

C4ConsoleQtCircle::C4ConsoleQtCircle(class C4Object *for_obj, C4PropList *props, const C4Value &val)
	: C4ConsoleQtShape(for_obj, props), radius(10), cx(0), cy(0), can_move_center(false)
{
	if (props)
	{
		can_move_center = props->GetPropertyBool(P_CanMoveCenter);
	}
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

bool C4ConsoleQtCircle::IsHit(int32_t x, int32_t y, int32_t hit_range, Qt::CursorShape *drag_cursor, int32_t *drag_border)
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

void C4ConsoleQtCircle::C4ConsoleQtCircle::Drag(int32_t x, int32_t y, int32_t dx, int32_t dy)
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


/* Shape list */

C4ConsoleQtShape *C4ConsoleQtShapes::CreateShape(class C4Object *for_obj, C4PropList *props, const C4Value &val)
{
	C4String *type = props->GetPropertyStr(P_Type);
	if (!type) return nullptr;
	C4ConsoleQtShape *shape = nullptr;
	if (type->GetData() == "rect") shape = new C4ConsoleQtRect(for_obj, props, val);
	else if (type->GetData() == "circle") shape = new C4ConsoleQtCircle(for_obj, props, val);
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
	if (dragging_shape == shape) dragging_shape = NULL;
}

void C4ConsoleQtShapes::ClearShapes()
{
	shapes.clear();
	dragging_shape = NULL;
	drag_cursor = Qt::CursorShape::ArrowCursor;
}

void C4ConsoleQtShapes::Draw(C4TargetFacet &cgo)
{
	// Draw all shapes with at least 1px line width
	ZoomDataStackItem zdsi(cgo.X, cgo.Y, cgo.Zoom);
	float line_width = std::max<float>(1.0f, 1.0f / cgo.Zoom);
	for (auto &shape : shapes) shape->Draw(cgo, line_width);
}

bool C4ConsoleQtShapes::MouseDown(float x, float y, float hit_range)
{
	// Check for shape hit and start dragging if a shape is in hit range
	int32_t hit_range_int = std::max(int32_t(hit_range + 0.5f), 1); // Using integer hit ranges for now
	// Ensure no leftover other shape
	if (dragging_shape) MouseUp(x, y);
	int32_t drag_border=-1;
	for (auto &shape : shapes)
	{
		if (shape->IsHit(x, y, hit_range_int, &drag_cursor, &drag_border))
		{
			dragging_shape = shape.get();
			dragging_shape->StartDragging(drag_border);
			drag_x = x;
			drag_y = y;
			return true;
		}
	}
	return false;
}

void C4ConsoleQtShapes::MouseMove(float x, float y, bool left_down, float hit_range)
{
	// Check for shape hit and start dragging if a shape is in hit range
	int32_t hit_range_int = std::max(int32_t(hit_range + 0.5f), 1); // Using integer hit ranges for now
	// move down move: Execute shape dragging (full pixels only)
	if (dragging_shape && left_down)
	{
		int32_t dx = int32_t(round(x - drag_x)),
		        dy = int32_t(round(y - drag_y));
		if (dx || dy)
		{
			drag_x += dx;
			drag_y += dy;
			dragging_shape->Drag(drag_x, drag_y, dx, dy);
		}
	}
	else if (!left_down)
	{
		// Just moving around: Update cursor
		drag_cursor = Qt::CursorShape::ArrowCursor;
		int32_t ignored;
		for (auto &shape : shapes) if (shape->IsHit(x, y, hit_range_int, &drag_cursor, &ignored)) break;
	}
	else
	{
		// Regular move: Reset drag cursor
		drag_cursor = Qt::CursorShape::ArrowCursor;
	}
}

void C4ConsoleQtShapes::MouseUp(float x, float y)
{
	// Stop dragging
	if (dragging_shape)
	{
		dragging_shape->emit ShapeDragged();
		dragging_shape->StopDragging();
		dragging_shape = NULL;
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
