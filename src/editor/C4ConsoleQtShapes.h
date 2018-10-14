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

#include "C4Include.h" // needed for automoc
#include "editor/C4ConsoleGUI.h" // for glew.h
#include "editor/C4ConsoleQt.h"
#include "script/C4Value.h"

// Shape base class
class C4ConsoleQtShape : public QObject
{
	Q_OBJECT
protected:
	C4Value rel_obj; // Object relative to which shape is defined
	C4Value properties;
	bool is_relative;
	int32_t dragging_border, selected_border;
	uint32_t border_color;
	const class C4PropertyDelegateShape *parent_delegate;
	class C4ConsoleQtShapes *shape_list;

protected:
	// Return shape color, or dragged border color if index is the border currently being dragged
	uint32_t GetBorderColor(int32_t border_index, bool dragging_border_is_bitmask, uint32_t default_color = 0u) const;
public:
	C4ConsoleQtShape(class C4Object *for_obj, C4PropList *props, const class C4PropertyDelegateShape *parent_delegate, class C4ConsoleQtShapes *shape_list);

	virtual bool IsHit(int32_t x, int32_t y, int32_t hit_range, Qt::CursorShape *drag_cursor, int32_t *drag_border, bool shift_down, bool ctrl_down) = 0;
	virtual void Draw(class C4TargetFacet &cgo, float line_width) = 0;

	// Coordinate transform: Add object
	int32_t AbsX(int32_t rel_x=0) const;
	int32_t AbsY(int32_t rel_y=0) const;

	// Start/stop dragging
	virtual bool StartDragging(int32_t *border, int32_t x, int32_t y, bool shift_down, bool ctrl_down) { dragging_border = *border; return true; }
	virtual void StopDragging();
	virtual void Drag(int32_t x, int32_t y, int32_t dx, int32_t dy, int32_t hit_range, Qt::CursorShape *drag_cursor) = 0;
	bool IsDragging() const { return dragging_border != -1; }

	virtual void SetValue(const C4Value &val) = 0;
	virtual C4Value GetValue() const = 0; 	// Return current shape as C4Value to be stored back to property

	const class C4PropertyDelegateShape *GetParentDelegate() const { return parent_delegate; }
	const C4PropList *GetProperties() const { return properties.getPropList(); }

	virtual bool IsSelectionAllowed(int32_t border) const { return false; }
	bool Select(int32_t border);
	void ResetSelection();
	virtual bool GetSelectedData(const C4Value &shape_val, const class C4PropertyPath &shape_property_path, C4PropList **shape_item_editorprops, C4PropList **shape_item_value, C4String **shape_item_name, class C4PropertyPath *shape_item_target_path) const
	{
		return false;
	}

	// Specialization
	virtual class C4ConsoleQtGraph *GetGraphShape() { return nullptr; }

signals:
	void ShapeDragged();
	void BorderSelectionChanged();
};	

// Rectangular shape
class C4ConsoleQtRect : public C4ConsoleQtShape
{
private:
	int32_t left, top, right, bottom;
	bool store_as_proplist;
	bool properties_lowercase;
public:
	C4ConsoleQtRect(class C4Object *for_obj, C4PropList *props, const class C4PropertyDelegateShape *parent_delegate, class C4ConsoleQtShapes *shape_list);

	bool IsHit(int32_t x, int32_t y, int32_t hit_range, Qt::CursorShape *drag_cursor, int32_t *drag_border, bool shift_down, bool ctrl_down) override;
	void Draw(class C4TargetFacet &cgo, float line_width) override;
	void Drag(int32_t x, int32_t y, int32_t dx, int32_t dy, int32_t hit_range, Qt::CursorShape *drag_cursor) override;

	void SetValue(const C4Value &val) override;
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
	C4ConsoleQtCircle(class C4Object *for_obj, C4PropList *props, const class C4PropertyDelegateShape *parent_delegate, class C4ConsoleQtShapes *shape_list);

	bool IsHit(int32_t x, int32_t y, int32_t hit_range, Qt::CursorShape *drag_cursor, int32_t *drag_border, bool shift_down, bool ctrl_down) override;
	void Draw(class C4TargetFacet &cgo, float line_width) override;
	void Drag(int32_t x, int32_t y, int32_t dx, int32_t dy, int32_t hit_range, Qt::CursorShape *drag_cursor) override;

	void SetValue(const C4Value &val) override;
	C4Value GetValue() const override;
};

// Single position shape
class C4ConsoleQtPoint : public C4ConsoleQtShape
{
private:
	int32_t cx, cy;
	bool horizontal_fix{ false };
	bool vertical_fix{ false };
public:
	C4ConsoleQtPoint(class C4Object *for_obj, C4PropList *props, const class C4PropertyDelegateShape *parent_delegate, class C4ConsoleQtShapes *shape_list);

	bool IsHit(int32_t x, int32_t y, int32_t hit_range, Qt::CursorShape *drag_cursor, int32_t *drag_border, bool shift_down, bool ctrl_down) override;
	void Draw(class C4TargetFacet &cgo, float line_width) override;
	void Drag(int32_t x, int32_t y, int32_t dx, int32_t dy, int32_t hit_range, Qt::CursorShape *drag_cursor) override;

	void SetValue(const C4Value &val) override;
	C4Value GetValue() const override;
};

// Vertices and edges
class C4ConsoleQtGraph : public C4ConsoleQtShape
{
	Q_OBJECT
protected:
	struct Vertex
	{
		int32_t x{0}, y{0};
		uint32_t color{0u}; // 0 = default color

		Vertex() = default;
	};

	struct Edge
	{
		int32_t vertex_indices[2]; // index into vertices array
		uint32_t color{0u}; // 0 = default color
		uint32_t line_thickness{1};

		Edge() { vertex_indices[0] = vertex_indices[1] = -1; }
		bool connects_to(int32_t vertex_index) const;
		bool connects_to(int32_t vertex_index, int32_t *idx) const;
	};

	// Actual vertex and edge data
	struct GraphData
	{
		std::vector<Vertex> vertices;
		std::vector<Edge> edges;

		// Convert elements to/from C4Value
		C4ValueArray *GetVerticesValue() const;
		C4ValueArray *GetEdgesValue() const;
		void SetVerticesValue(const C4ValueArray *vvertices);
		void SetEdgesValue(const C4ValueArray *vedges);

		int32_t GetEdgeCountForVertex(int32_t vertex_index) const;

		// Graph modification. Called from C4ConsoleQtGraph::EditGraph only, which propagates the change to the value via the queue
		void SetVertexPos(int32_t vertex_index, int32_t new_x, int32_t new_y);
		void EditEdge(int32_t edge_index, int32_t change_vertex_index, int32_t new_vertex_index);
		void InsertEdgeBefore(int32_t insert_edge_index, int32_t vertex1, int32_t vertex2);
		void InsertVertexBefore(int32_t insert_vertex_index, int32_t x, int32_t y);
		void RemoveEdge(int32_t edge_index);
		void RemoveVertex(int32_t vertex_index);

		// Graph modification on internal C4Value script data. Called from C4ConsoleQtGraph::EditGraphValue.
		static void EditGraphValue_SetVertexPos(C4ValueArray *vvertices, int32_t vertex_index, int32_t new_x, int32_t new_y);
		static void EditGraphValue_EditEdge(C4ValueArray *vvertices, C4ValueArray *vedges, int32_t edge_index, int32_t change_vertex_index, int32_t new_vertex_index);
		static void EditGraphValue_InsertEdgeBefore(C4ValueArray *vvertices, C4ValueArray *vedges, int32_t insert_edge_index, int32_t vertex1, int32_t vertex2);
		static void EditGraphValue_InsertVertexBefore(C4ValueArray *vvertices, C4ValueArray *vedges, int32_t insert_vertex_index, int32_t x, int32_t y);
		static void EditGraphValue_RemoveEdge(C4ValueArray *vvertices, C4ValueArray *vedges, int32_t edge_index);
		static void EditGraphValue_RemoveVertex(C4ValueArray *vvertices, C4ValueArray *vedges, int32_t vertex_index);
		static bool EditGraphValue_EdgeConnectsTo(C4PropList *edge, int32_t vertex_index);
	} graph;

	bool allow_vertex_selection = false; // If vertices on the graph can be selected
	bool allow_edge_selection = false; // If edges on the graph can be selected
	bool horizontal_fix = false; // If edges are locked horizontally
	bool vertical_fix = false; // If edges are locked vertically
	bool structure_fix = false; // If edge+vertex insertion/deletion is blocked
	bool draw_arrows = false;   // If directions on edges are to be signified with arrowheads

	C4Value vertex_delegate, edge_delegate;

	// Drag snap to other vertices
	int32_t drag_snap_offset_x = 0, drag_snap_offset_y = 0;
	bool drag_snapped = false;
	int32_t drag_snap_vertex = -1, drag_source_vertex_index = -1;

	// Resolve border_index to vertices/edges: Use negative indices for edges and zero/positive indices for vertices
	static bool IsVertexDrag(int32_t border) { return border >= 0; }
	static bool IsEdgeDrag(int32_t border) { return border < -1; }
	static int32_t DragBorderToVertex(int32_t border) { return border; }
	static int32_t DragBorderToEdge(int32_t border) { return -2 - border; }
	static int32_t VertexToDragBorder(int32_t vertex) { return vertex; }
	static int32_t EdgeToDragBorder(int32_t edge) { return -edge - 2; }

	void DrawEdge(class C4TargetFacet &cgo, const Vertex &v0, const Vertex &v2, uint32_t clr, float line_width, float edge_width, bool highlight);
public:
	C4ConsoleQtGraph(class C4Object *for_obj, C4PropList *props, const class C4PropertyDelegateShape *parent_delegate, class C4ConsoleQtShapes *shape_list);

	bool IsHit(int32_t x, int32_t y, int32_t hit_range, Qt::CursorShape *drag_cursor, int32_t *drag_border, bool shift_down, bool ctrl_down) override;
	void Draw(class C4TargetFacet &cgo, float line_width) override;
	void Drag(int32_t x, int32_t y, int32_t dx, int32_t dy, int32_t hit_range, Qt::CursorShape *drag_cursor) override;
	bool StartDragging(int32_t *border, int32_t x, int32_t y, bool shift_down, bool ctrl_down) override;
	void StopDragging() override;

	void SetValue(const C4Value &val) override;
	C4Value GetValue() const override;

	bool IsSelectionAllowed(int32_t border) const override;
	bool GetSelectedData(const C4Value &shape_val, const class C4PropertyPath &shape_property_path, C4PropList **shape_item_editorprops, C4PropList **shape_item_value, C4String **shape_item_name, class C4PropertyPath *shape_item_target_path) const override;

	void EditGraph(bool signal_change, C4ControlEditGraph::Action action, int32_t index, int32_t x, int32_t y);
	static void EditGraphValue(C4Value &val, C4ControlEditGraph::Action action, int32_t index, int32_t x, int32_t y);

	class C4ConsoleQtGraph *GetGraphShape() override { return this; }

protected:
	void EditEdge(int32_t edge_index, int32_t change_vertex_index, int32_t new_vertex);
	int32_t AddVertex(int32_t new_x, int32_t new_y);
	int32_t AddEdge(int32_t connect_vertex_index_1, int32_t connect_vertex_index_2);
	void InsertVertexBefore(int32_t insert_vertex_index, int32_t x, int32_t y);
	void InsertEdgeBefore(int32_t insert_edge_index, int32_t connect_vertex_index_1, int32_t connect_vertex_index_2);
	virtual int32_t InsertVertexOnEdge(int32_t split_edge_index, int32_t x, int32_t y);
	virtual int32_t InsertVertexOnVertex(int32_t target_vertex_index, int32_t x, int32_t y);
	virtual void RemoveEdge(int32_t edge_index);
	virtual void RemoveVertex(int32_t vertex_index, bool create_skip_connection);

	virtual bool IsPolyline() const { return false; }

	// Check if given vertex/edge can be modified under given shift state
	virtual bool IsVertexHit(int32_t vertex_index, Qt::CursorShape *drag_cursor, bool shift_down, bool ctrl_down);
	virtual bool IsEdgeHit(int32_t edge_index, Qt::CursorShape *drag_cursor, bool shift_down, bool ctrl_down);

signals:
	void GraphEdit(C4ControlEditGraph::Action action, int32_t index, int32_t x, int32_t y);
};

// Specialization of graph: One line of connected vertices
class C4ConsoleQtPolyline : public C4ConsoleQtGraph
{
private:
	bool start_from_object = false;
public:
	C4ConsoleQtPolyline(class C4Object *for_obj, C4PropList *props, const class C4PropertyDelegateShape *parent_delegate, class C4ConsoleQtShapes *shape_list);

	void Draw(class C4TargetFacet &cgo, float line_width) override;
	void SetValue(const C4Value &val) override;
	C4Value GetValue() const override;

protected:
	int32_t InsertVertexOnEdge(int32_t split_edge_index, int32_t x, int32_t y) override;
	int32_t InsertVertexOnVertex(int32_t target_vertex_index, int32_t x, int32_t y) override;
	void RemoveEdge(int32_t edge_index) override;

	bool IsVertexHit(int32_t vertex_index, Qt::CursorShape *drag_cursor, bool shift_down, bool ctrl_down) override;
	bool IsPolyline() const override { return true; }
};

// Specialization of graph: One closed line of connected vertices
class C4ConsoleQtPolygon : public C4ConsoleQtPolyline
{
public:
	C4ConsoleQtPolygon(class C4Object *for_obj, C4PropList *props, const class C4PropertyDelegateShape *parent_delegate, class C4ConsoleQtShapes *shape_list)
		: C4ConsoleQtPolyline(for_obj, props, parent_delegate, shape_list) {}

	void SetValue(const C4Value &val) override;

protected:
	int32_t InsertVertexOnEdge(int32_t split_edge_index, int32_t x, int32_t y) override;
	int32_t InsertVertexOnVertex(int32_t target_vertex_index, int32_t x, int32_t y) override;
	void RemoveEdge(int32_t edge_index) override;

	bool IsVertexHit(int32_t vertex_index, Qt::CursorShape *drag_cursor, bool shift_down, bool ctrl_down) override;
};

/* List of all current editable Qt shapes */
class C4ConsoleQtShapes
{
	typedef std::list<std::unique_ptr<C4ConsoleQtShape> > ShapeList;
	ShapeList shapes;
	C4ConsoleQtShape *dragging_shape{nullptr}, *selected_shape{nullptr};
	Qt::CursorShape drag_cursor{Qt::CursorShape::ArrowCursor};
	float drag_x{0}, drag_y{0};
public:
	C4ConsoleQtShapes() = default;

	C4ConsoleQtShape *CreateShape(class C4Object *for_obj, C4PropList *props, const C4Value &val, const class C4PropertyDelegateShape *parent_delegate);
	void AddShape(C4ConsoleQtShape *shape);
	void RemoveShape(C4ConsoleQtShape *shape);
	void ClearShapes();

	// Mouse callbacks from viewports to execute shape dragging
	bool MouseDown(float x, float y, float hit_range, bool shift_down, bool ctrl_down); // return true if a shape was hit
	void MouseMove(float x, float y, bool left_down, float hit_range, bool shift_down, bool ctrl_down); // move move: Execute shape dragging
	void MouseUp(float x, float y, bool shift_down, bool ctrl_down);

	void Draw(C4TargetFacet &cgo);

	// Dragging info
	bool HasDragCursor() const { return drag_cursor != Qt::CursorShape::ArrowCursor; }
	Qt::CursorShape GetDragCursor() const { return drag_cursor; }
	bool IsDragging() const { return !!dragging_shape; }

	// Selected shapes
	void SetSelectedShape(C4ConsoleQtShape *new_selection, int32_t selected_border);
	C4ConsoleQtShape *GetSelectedShape() const { return selected_shape; }
	bool GetSelectedShapeData(const C4Value &shape_val, const class C4PropertyPath &shape_property_path, C4PropList **shape_item_editorprops, C4PropList **shape_item_value, C4String **shape_item_name, class C4PropertyPath *shape_item_target_path) const;
};

/* Shape holder class: Handles adding/removal of shape to shapes list */
class C4ConsoleQtShapeHolder
{
	C4ConsoleQtShape *shape{nullptr};
	bool last_visit;
	C4Value last_value;

	static bool last_visit_flag;

public:
	C4ConsoleQtShapeHolder()  = default;
	~C4ConsoleQtShapeHolder() { Clear(); }

	void Clear();
	void Set(C4ConsoleQtShape *new_shape);
	C4ConsoleQtShape *Get() const { return shape; }

	// Check counter to determine unused shapes
	void visit() { last_visit = last_visit_flag; }
	bool was_visited() const { return last_visit == last_visit_flag; }
	static void begin_visit() { last_visit_flag = !last_visit_flag; }

	// Remember pointer to last proplist or array set in value to reflect scripted updates
	const C4Value &GetLastValue() const { return last_value; }
	void SetLastValue(const C4Value &new_val) { last_value = new_val; }
};


#endif // WITH_QT_EDITOR
#endif // INC_C4ConsoleQtShapes
