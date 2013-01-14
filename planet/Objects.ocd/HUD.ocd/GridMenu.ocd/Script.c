/**
	GridMenu
	A simple grid-menu based on the Library_Menu implementation.
	It tries to creaty square-ish menus.
	
	@author boni
*/

#include Library_Menu


// default values used for the menu
local menu_itemwidth; // the size of one item to be displayed.
local menu_spacing; // the space between items
local menu_border; // the space from the edge to the first item

// how many items should be in one row
local rowitemcount;

/** Sets how many items should be displayed in one row (equals column-count). */
public func SetRowItemCount(int itemcount) { rowitemcount = itemcount; }

/** Sets the distance between items in the grid */
public func SetMenuSpacing(int distance) { menu_spacing = distance; }

/** Sets the distance from the outer edges to the outer objects */
public func SetMenuBorder(int distance) { menu_border = distance; }

/** Sets the background color of the menu */
public func SetBackgroundColor(int rgb) { SetClrModulation(rgb, 1); }

public func Construction()
{
	rowitemcount = nil;
	menu_itemwidth = 96;
	menu_border = menu_itemwidth/3;
	menu_spacing = 40;
	
	SetGraphics(nil, GUI_GridMenu, 1, GFXOV_MODE_Base);
	
	_inherited(...);
}

public func UpdateMenu()
{
	var basex = GetX();
	var basey = GetY();
	
	var rc; // rowcount
	var ric; // row item count. how many items are in one row (== column count)
	var itemcount = GetItemCount();
	 
	// calc size if needed
	ric = rowitemcount;
	if(ric == nil)
	{
		// we use the square root to get a square layout! trololo!
		// must not be 0
		ric = Max(1, Sqrt(itemcount));
		rc = ric;
		if(itemcount > rc*rc)
			ric += 1;
	}
	else
	{
		rc = itemcount/rowitemcount;
	}
	// adjust row-count if needed
	if(itemcount > rc*ric)
		rc += 1;
	
	// set the background
	var size = GetMenuSize(itemcount, rc, ric); // == [width,height]
	SetShape(0,0,size[0],size[1]);
	SetObjDrawTransform(size[0]*1000, 0, (size[0]*1000), 0, size[1]*1000, (size[1]*1000), 1);
	SetPosition(basex,basey);
	
	
	// order the items
	var index = 0;
	var x,y;
	x = basex + menu_itemwidth/2 + menu_border;
	y = basey + menu_itemwidth/2 + menu_border;
	for(var i = 0; i < rc; i++)
	{
		for(var j = 0; j < ric; j++)
		{
			// we might have more slots than needed
			if(index >= itemcount)
				break;
			var pos = GetItemPosition(index, i, j); // ==[x,y]
			var item = GetItem(index);
			item->SetPosition(x+pos[0],y+pos[1]);

			index++;
		}
	}
}

/** Returns the relative position of the index'th item in the menu. The return value does not take the border into account.
    If you just want to change the distances, use SetMenuBorder and SetMenuSpacing.
    @param index the index of the object in the menu.
    @param row in which row the object should be positioned. Starts with 0.
    @param column in which column the object should be positioned. Starts with 0.
    @returns an coordinate containing the relative x/y position: [x,y] 
*/ 
protected func GetItemPosition(int index, int row, int column)
{
	var x,y;
	x = (menu_itemwidth + menu_spacing) * column;
	y = (menu_itemwidth + menu_spacing) * row;
	
	return [x,y];
}

/** Returns the width and height of the menu.
    @param count the amount of items in the menu
    @param rows the amount of rows the menu should have
    @param columns the amount of columns the menu should have
*/
protected func GetMenuSize(int count, int rows, int columns)
{
	var w,h;
	
	w = (menu_itemwidth + menu_spacing)*columns - menu_spacing; // with n items, we have n-1 spaces, therefore we have to substract one
	h = (menu_itemwidth + menu_spacing)*rows - menu_spacing;
	w += 2*menu_border;
	h += 2*menu_border;
	
	return [w,h];
}