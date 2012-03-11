/**
	FancyGridMenu
	A more fancier grid menu, that has the focussed (hovered) item popping out.

	@author boni
*/

#include GUI_GridMenu

// the item that is beeing hovered over, if any
local focussed_item;
local focussed_index;

local rowcount, colcount; // we're saving the last amount of columns/rows
local menu_spacing_focussed; // array containing the x and y spacing when an item is focussed
local menu_focussed_size; 

public func Construction()
{
	// initialize base variables
	_inherited(...);
	
	menu_focussed_size = (menu_itemwidth*3)/2;
	focussed_item = nil;
	menu_spacing_focussed = CreateArray(2);
}

// take actions needed for focussed item awesomeness
// The item size is doubled, all the spacing around it is reduced to so little, that the item still stays at the same position.
protected func GetItemPosition(int index, int row, int column)
{ 
	if(focussed_item == nil || focussed_index == index)
		return inherited(index, row, column);
	
	var x,y; 
	var focussed_col = focussed_index%colcount;
	var focussed_row = focussed_index/colcount;

	// size of the focussed item	
	var dif = menu_focussed_size-menu_itemwidth; // difference in size to the other items

	// we use reduced spacing when an item is selected
	x = (menu_itemwidth + menu_spacing_focussed[0]) * column;
	y = (menu_itemwidth + menu_spacing_focussed[1]) * row;
	
	// but have to calc in an offset if it's past the focussed item
	if(column == focussed_col)
		x += dif*column/colcount;
	else if(column > focussed_col)
		x += dif;
	
	if(row == focussed_row)
		y += dif*row/rowcount;
	else if(row > focussed_row)
		y += dif;
	
	return [x,y];
	
	// TODO: remove old stuff.
	
	/*
	// we take the distance to the focussed item, and make it bigger the closer we are to it. Max range = no change
	
	// take the difference to the outer reference wall, and move it a fraction of that.
	
	var factor = 5;
	
	if(column < focussed_col)
		x = non_focus_pos[0] - non_focus_pos[0]/factor;
	else if(column > focussed_col)
		x = non_focus_pos[0] + ((menu_itemwidth + menu_spacing)*(colcount-1)-non_focus_pos[0])/factor;
	else
		x = non_focus_pos[0];
	
	if(row < focussed_row)
		y = non_focus_pos[1] - non_focus_pos[1]/factor;
	else if(row > focussed_row)
		y = non_focus_pos[1] + ((menu_itemwidth + menu_spacing)*(rowcount-1)-non_focus_pos[1])/factor;
	else
		y = non_focus_pos[1];
	
	return [x,y];
	*/
	/*
	// just move outwards! \o/
	// first calc the distance we have to move. the closer the more. the most outer items shouldn't move at all. (at least not outwards)
	// we just do a linear interpolation to see how far we are away from the edge, and apply that value to a curve then
	// we know that the most left item has pos 0, and the most right has pos menusize[x/y]
	var xd = 0, yd = 0; // distance from the focussed item
	var xref = 0, yref = 0; // distance to the reference edge
	// xd = 0 if they're in the same column
	if(non_focus_pos[0] < focussed_pos[0]) // item to the left -> left edge (=0)
	{
		xd = focussed_pos[0]-non_focus_pos[0];//-0
		xref = focussed_pos[0];
		x = focussed_pos[0] - Sin(90 - (90*xd)/xref, xref);
	}
	else if(non_focus_pos[0] > focussed_pos[0]) // item to the right -> right edge
	{
		xd = non_focus_pos[0]-focussed_pos[0];
		xref = menusize[0]-focussed_pos[0];
		x = focussed_pos[0] + Sin(90 - (90*xd)/xref, xref);
	}
	else // directly above/below
		x = focussed_pos[0];
	
	// yd = 0 if they're in the same row
	if(non_focus_pos[1] < focussed_pos[1]) // item above -> top edge (=0)
	{
		yd = focussed_pos[1]-non_focus_pos[1];//-0
		yref = focussed_pos[1];
		y = focussed_pos[1] - Sin(90 - (90*yd)/yref, yref);
	}
	else if(non_focus_pos[1] > focussed_pos[1]) // item below -> bottom edge
	{
		yd = non_focus_pos[1]-focussed_pos[1];
		yref = menusize[1]-focussed_pos[1];
		y = focussed_pos[1] + Sin(90 - (90*yd)/yref, yref);
	}
	else // in the same row
		y = focussed_pos[1];
	
	return [x,y];
	
	// adjust side
	if(non_focus_pos[0] > focussed_pos[0])
		x = menusize[0]-x;
	if(non_focus_pos[1] > focussed_pos[1])
		y = menusize[1]-y;
	*/
	//Log("Focussed: %d %d", focussed_col, focussed_row);
	
	// spacing: reduce the total spacing by the amount we zoom up the focussed object (we double the size -> -1 size)
	/*
	var spacing = menu_spacing * (colcount-1) - focussed_size/2;
	
	x = (menu_itemwidth + spacing) * column;
	y = (menu_itemwidth + spacing) * row;
	
	// adjust horizontal spacing
	if(column > 0 && column == focussed_col)
		x += focussed_size/2;
	if(column > focussed_col)
		x += focussed_size;
	
	// adjust vertical spacing
	if(row > 0 && row == focussed_row)
		y += focussed_size/2;
	if(row > focussed_row)
		y += focussed_size;
	
	return [x,y];
	*/
}

protected func GetMenuSize(int count, int rows, int columns)
{
	// we save the last menu size to save calculation stuff.
	rowcount = rows;
	colcount = columns;
	
	// calculate focussed spacing
	menu_spacing_focussed[0] = (colcount*menu_spacing - (menu_focussed_size-menu_itemwidth))/colcount;
	menu_spacing_focussed[1] = (rowcount*menu_spacing - (menu_focussed_size-menu_itemwidth))/rowcount;
	
	return _inherited(count, rows, columns);
}

// Called if the mouse cursor enters hovering over an item.
public func OnMouseOverItem(object over_item, object dragged_item)
{
	focussed_item = over_item;
	//focussed_item->SetObjDrawTransform(2000,0,0,0,2000,0, 1);
	focussed_item->SetSize((menu_itemwidth*3)/2);
	
	focussed_index = GetIndexOf(menu_items, focussed_item);
	
	UpdateMenu();
	
	return _inherited(over_item, dragged_item, ...);
}

// Called if the mouse cursor exits hovering over an item.
public func OnMouseOutItem(object out_item, object dragged_item)
{
	//focussed_item->SetObjDrawTransform(1000,0,0,0,1000,0, 1);
	focussed_item->SetSize(menu_itemwidth);
	focussed_item = nil;
	
	focussed_index = nil;
	
	UpdateMenu();
	
	return _inherited(out_item, dragged_item, ...);
}