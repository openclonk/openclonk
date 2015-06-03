/*-- Editor corner for dragging --*/
// Usage:
// RangeMarker->Create(int x, int y, int rotation, proplist callback_obj, callback_function, callback_param)
// Creates a range marker at the given position and rotation (top left: r=0, top right: r=90, etc.)
// When the marker is moved in editor mode, it will call callback_obj->callback_function(callback_param, new_x, new_y)

local cb_obj, cb_fn, cb_par; // callback object and function if self is moved
local lx,ly; // last x/y coordinates
local EditCursorCommands = ["OnMoved()"];

// definition call: create range marker
func Create(int x, int y, int rotation, proplist callback_obj, callback_function, callback_param)
{
	var obj = CreateObject(RangeMarker, x, y);
	obj->Init(rotation, callback_obj, callback_function, callback_param);
	return obj;
}

func Init(int r, proplist callback_obj, callback_function, callback_param)
{
	SetR(r);
	cb_obj = callback_obj;
	cb_fn = callback_function;
	cb_par = callback_param;
	lx = GetX();
	ly = GetY();
	return true;
}

func OnMoved()
{
	lx = GetX(); ly = GetY();
	if (!cb_obj) return RemoveObject();
	return cb_obj->Call(cb_fn, cb_par, lx, ly);
}

func EditCursorDeselection(object new_selection)
{
	// If new selection is neither this nor the taget object, ensure target removes any editor markers
	if (cb_obj && new_selection != cb_obj) cb_obj->EditCursorDeselection(new_selection);
	return true;
}

func EditCursorMoved(int old_x, int old_y)
{
	return OnMoved();
}

// Will be recreated when needed.
func SaveScenarioObject() { return false; }

