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

#ifndef INC_C4ConsoleQtShapes
#define INC_C4ConsoleQtShapes
#ifdef WITH_QT_EDITOR

#include "editor/C4ConsoleGUI.h" // for glew.h
#include "editor/C4ConsoleQt.h"
#include "script/C4Value.h"

// Shape base class
class C4ConsoleQtShape : public QObject
{
	Q_OBJECT
protected:
	C4Value rel_obj; // Object relative to which shape is defined
	bool is_relative;
	int32_t dragging_border;
	uint32_t border_color;

protected:
	// Return shape color, or dragged border color if index is the border currently being dragged
	uint32_t GetBorderColor(int32_t border_index, bool dragging_border_is_bitmask) const;
public:
	C4ConsoleQtShape(class C4Object *for_obj, C4PropList *props);

	virtual bool IsHit(int32_t x, int32_t y, int32_t hit_range, Qt::CursorShape *drag_cursor, int32_t *drag_border) = 0;
	virtual void Draw(class C4TargetFacet &cgo, float line_width) = 0;

	// Coordinate transform: Add object
	int32_t AbsX(int32_t rel_x) const;
	int32_t AbsY(int32_t rel_x) const;

	// Start/stop dragging
	void StartDragging(int32_t border) { dragging_border = border; }
	void StopDragging() { dragging_border = -1; }
	virtual void Drag(int32_t x, int32_t y, int32_t dx, int32_t dy) = 0;
	bool IsDragging() const { return dragging_border != -1; }

	// Return current shape as C4Value to be stored back to property
	virtual C4Value GetValue() const = 0;

signals:
	void ShapeDragged();
};	

// Rectangular shape
class C4ConsoleQtRect : public C4ConsoleQtShape
{
private:
	int32_t left, top, right, bottom;
public:
	C4ConsoleQtRect(class C4Object *for_obj, C4PropList *props, const C4Value &val);

	bool IsHit(int32_t x, int32_t y, int32_t hit_range, Qt::CursorShape *drag_cursor, int32_t *drag_border) override;
	void Draw(class C4TargetFacet &cgo, float line_width) override;
	void Drag(int32_t x, int32_t y, int32_t dx, int32_t dy) override;

	C4Value GetValue() const override;
};

// Circular shape
class C4ConsoleQtCircle : public C4ConsoleQtShape
{
private:
	int32_t radius;
	int32_t cx, cy;
	bool can_move_center;
public:
	C4ConsoleQtCircle(class C4Object *for_obj, C4PropList *props, const C4Value &val);

	bool IsHit(int32_t x, int32_t y, int32_t hit_range, Qt::CursorShape *drag_cursor, int32_t *drag_border) override;
	void Draw(class C4TargetFacet &cgo, float line_width) override;
	void Drag(int32_t x, int32_t y, int32_t dx, int32_t dy) override;

	C4Value GetValue() const override;
};

/* List of all current editable Qt shapes */
class C4ConsoleQtShapes
{
	typedef std::list<std::unique_ptr<C4ConsoleQtShape> > ShapeList;
	ShapeList shapes;
	C4ConsoleQtShape *dragging_shape;
	Qt::CursorShape drag_cursor;
	float drag_x, drag_y;
public:
	C4ConsoleQtShapes() : dragging_shape(nullptr), drag_x(0), drag_y(0), drag_cursor(Qt::CursorShape::ArrowCursor) { }

	C4ConsoleQtShape *CreateShape(class C4Object *for_obj, C4PropList *props, const C4Value &val);
	void AddShape(C4ConsoleQtShape *shape);
	void RemoveShape(C4ConsoleQtShape *shape);
	void ClearShapes();

	// Mouse callbacks from viewports to execute shape dragging
	bool MouseDown(float x, float y, float hit_range); // return true if a shape was hit
	void MouseMove(float x, float y, bool left_down, float hit_range); // move move: Execute shape dragging
	void MouseUp(float x, float y);

	void Draw(C4TargetFacet &cgo);

	// Dragging info
	bool HasDragCursor() const { return drag_cursor != Qt::CursorShape::ArrowCursor; }
	Qt::CursorShape GetDragCursor() const { return drag_cursor; }
};

/* Shape holder class: Handles adding/removal of shape to shapes list */
class C4ConsoleQtShapeHolder
{
	C4ConsoleQtShape *shape;

public:
	C4ConsoleQtShapeHolder() : shape(nullptr) {}
	~C4ConsoleQtShapeHolder() { Clear(); }

	void Clear();
	void Set(C4ConsoleQtShape *new_shape);
	C4ConsoleQtShape *Get() const { return shape; }
};


#endif // WITH_QT_EDITOR
#endif // INC_C4ConsoleQtShapes