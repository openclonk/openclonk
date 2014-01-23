/**
	Menu object
	Controls a menu consistent of a big circle with small circular menu items.
	Callbacks to the commander:
	* OnItemSelection(object menu, object item);
	* OnItemSelectionAlt(object menu, object item);
	* OnItemDropped(object menu, object drop_item, object on_item);
	* OnItemDragged(object menu, object drag_item, object on_item);
	
	@author Maikel, Newton, Mimmo
*/

// Menu functionality
#include Library_Menu

static const MENU_Radius = 140;

protected func Construction() {
	SetSymbol(nil);
	inherited(...);
}


/** Sets the symbol that is displayed in the menu background */
public func SetSymbol(symbol)
{
	SetGraphics("BG", this, 2, GFXOV_MODE_Base);

	if(!symbol)
	{
		SetGraphics(nil, nil, 1);

	}
	else
	{
		if (GetType(symbol) == C4V_C4Object)
			SetGraphics(nil, nil, 1, GFXOV_MODE_ObjectPicture, nil, nil, symbol);
		else
			SetGraphics(nil, symbol, 1, GFXOV_MODE_IngamePicture);
			
		SetObjDrawTransform(800, 0, 0, 0, 800, 0, 1);
	}
	return;
}



/** Determines the item position for the nth circle for a certain number of circles. */
private func GetItemPosition(int n, int total)
{
	// Safety.
	if (n > total)
		return;
	
	// Packing 7 or less circles.
	if (total <= 7)
	{
		if (n == 0)
			return [0, 0];
		else
		{	
			var x = -Cos(60 * (n+1), 2 * MENU_Radius / 3);
			var y = -Sin(60 * (n+1), 2 * MENU_Radius / 3);
			return [x, y];
		}
	}
	
	// Packing 19 or less circles.
	if (total <= 19)
	{
		if (n == 0)
			return [0, 0];
		else if (n < 7)
		{	
			var x = -Cos(60 * (n+1), 2 * MENU_Radius / 5);
			var y = -Sin(60 * (n+1), 2 * MENU_Radius / 5);
			return [x, y];
		}
		else
		{
			var x = -Cos(30 * (n-4) + 15, 31 * MENU_Radius / 40);
			var y = -Sin(30 * (n-4) + 15, 31 * MENU_Radius / 40);
			return [x, y];
		}		
	}
	
	// Packing 37 or less circles.
	if (total <= 37)
	{
		if (n == 0)
			return [0, 0];
		else if (n < 7)
		{	
			var x = -Cos(60 * (n+1), 2 * MENU_Radius / 7);
			var y = -Sin(60 * (n+1), 2 * MENU_Radius / 7);
			return [x, y];
		}
		else if (n < 19)
		{
			var x = -Cos(30 * (n-4) + 15, 31 * MENU_Radius / 56);
			var y = -Sin(30 * (n-4) + 15, 31 * MENU_Radius / 56);
			return [x, y];
		}	
		else
		{
			var x = -Cos(30 * (n-16), 61 * MENU_Radius / 72);
			var y = -Sin(30 * (n-16), 61 * MENU_Radius / 72);
			return [x, y];
		}		
	}
	// More cases are not covered yet.
	return;
}

/** Gives the radius for an item. */
private func GetItemRadius(int total)
{
	if (total <= 7)
		return MENU_Radius / 3;
	if (total <= 19)
		return MENU_Radius / 5;
	if (total <= 37)
		return MENU_Radius / 7;
	return 1;
}

/** Redraws the menu */
public func UpdateMenu()
{
	// Safety: check for items.
	var item_count = GetLength(menu_items);
	if (!item_count)
		return;
	
	var x = GetX();
	var y = GetY();
	
	for (var i = 0; i < item_count; i++)
	{
		var pos = GetItemPosition(i, item_count);
		var item = menu_items[i];
		if (item)
		{
			item->SetSize(200 * GetItemRadius(item_count) / 96);
			item->SetPosition(x + pos[0], y + pos[1]);
		}
	}
	return;
}

public func OnMouseOverItem(object item, object dragged)

{
	// make it appear bigger
	item->SetObjDrawTransform(1100, 0, 0, 0, 1100, 0, 1);
}

public func OnMouseOutItem(object item, object dragged)
{
	// return to normal size
	item->SetObjDrawTransform(1000, 0, 0, 0, 1000, 0, 1);
}
