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


local menu_commander; // Object controlling the menu, commands are passed to this object.
local menu_object; // Object for which the menu is shown.
local menu_items; // List of the items in the menu.
local menu_shown;
local dragdrop;

static const MENU_Radius = 140;


protected func Construction()
{
	menu_items = [];
	menu_shown = false;
	dragdrop = true;
	// Visibility
	this.Visibility = VIS_None;
	// Parallaxity
	this.Parallaxity = [0, 0];
	// Mouse drag image
	this.MouseDragImage = nil;
	this.MouseDrag = MD_NoClick | MD_DropTarget;
	SetSymbol(nil);
	return;
}

public func IsDragDropMenu() { return dragdrop; }

public func SetDragDropMenu(bool is_dragdrop)
{
	dragdrop = is_dragdrop;
	// Mouse drag image
	this.MouseDragImage = this;
	return;	
}

// Sets the commander of this menu.
public func SetCommander(object commander)
{
	menu_commander = commander;
	return;
}

// Returns the commander of this menu.
public func GetCommander()
{
	return menu_commander;
}

// Sets the menu object for this menu.
public func SetMenuObject(object menuobject)
{
	menu_object = menuobject;
	return;
}

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

/** Adds an item to this menu.
	@param symbol used to specify the symbol of the menu item, either an id or object.
	@param pos position in the menu where the item should be placed, \c nil for first open slot.
	@param amount the amount displayed next to the symbol.
	@return a pointer to the menu item created, or \c nil if failed.
*/
public func AddItem(object item, int pos)
{
	var item_cnt = GetLength(menu_items);
	// Find position if not specified.
	if (pos == nil)
	{
		pos = item_cnt;
		for (var i = 0; i < item_cnt; i++)
			if (!menu_items[i])
			{
				pos = i;
				break;
			}		
	}
	// Check if pos is already taken.
	else if (menu_items[pos])
		return false;
	
	// Create new menu item.
	menu_items[pos] = item;
	item->SetMenu(this);
	item->SetOwner(GetOwner());
	// Set item visibility.
	item.Visibility = VIS_None;
	if (menu_shown)
		item.Visibility = VIS_Owner;
	UpdateMenu();
	return true;
}

/** Gives the menu item at the specified position.
	@param the position in the menu.
	@return the menu item at the specified position.
*/
public func GetItem(int index)
{
	return menu_items[index];
}

/** Returns all menu items as an array
	@return array of all menu items
*/
public func GetItems()
{
	return menu_items;
}

/** Removes a menu item the menu.
	@item the position in the menu.
	@return the menu item at the specified position.
*/
public func RemoveItem(object item)
{
	for (var mitem in menu_items)
	{
		if (mitem = item)
			mitem->RemoveObject();	
	}
	UpdateMenu();
	return;
}

// Determines the item position for the nth circle for a certain number of circles.
private func GetItemPosition(int n, int total)
{
	// Safety.
	if (n > total)
		return;
	
	// Packing 7 or less circles.
	if (total <= 7)
	{
		if (n == 7)
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
		if (n == 7)
			return [0, 0];
		else if (n < 7)
		{	
			var x = -Cos(60 * (n+1), 2 * MENU_Radius / 5);
			var y = -Sin(60 * (n+1), 2 * MENU_Radius / 5);
			return [x, y];
		}
		else
		{
			var x = -Cos(30 * (n-5) + 15, 31 * MENU_Radius / 40);
			var y = -Sin(30 * (n-5) + 15, 31 * MENU_Radius / 40);
			return [x, y];
		}		
	}
	
	// Packing 37 or less circles.
	if (total <= 37)
	{
		if (n == 7)
			return [0, 0];
		else if (n < 7)
		{	
			var x = -Cos(60 * (n+1), 2 * MENU_Radius / 7);
			var y = -Sin(60 * (n+1), 2 * MENU_Radius / 7);
			return [x, y];
		}
		else if (n <= 19)
		{
			var x = -Cos(30 * (n-5) + 15, 31 * MENU_Radius / 56);
			var y = -Sin(30 * (n-5) + 15, 31 * MENU_Radius / 56);
			return [x, y];
		}	
		else
		{
			var x = -Cos(30 * (n-17), 61 * MENU_Radius / 72);
			var y = -Sin(30 * (n-17), 61 * MENU_Radius / 72);
			return [x, y];
		}		
	}
	// More cases are not covered yet.
	return;
}

// Gives the radius for an item.
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
		var pos = GetItemPosition(i + 1, item_count);
		var item = menu_items[i];
		if (item)
		{
			item->SetSize(200 * GetItemRadius(item_count) / 96);
			item->SetPosition(x + pos[0], y + pos[1]);
		}
	}
	return;
}

// Shows the menu.
public func Show()
{
	UpdateMenu();
	// Change visibility.
	for (var item in menu_items)
		if (item)
			item.Visibility = VIS_Owner;
	this.Visibility = VIS_Owner;
	menu_shown = true;
	return;
}

public func Hide()
{
	// Change visibility.
	for (var item in menu_items)
		if (item)
			item.Visibility = VIS_None;
	this.Visibility = VIS_None;
	CustomMessage("", this, menu_object->GetOwner());
	menu_shown = false;
	return;
}

// Engine callback: if the menu is destroyed, the items must follow.
protected func Destruction()
{
	for (var i = 0; i < GetLength(menu_items); i++)
		if (menu_items[i])
			menu_items[i]->RemoveObject();
	return;
}

/* Callbacks from the menu items, to be forwarded to the commander. */

// Called when an item has been selected (left mouse button).
public func OnItemSelection(object item)
{
	if (!menu_commander)
		return;
	// Forward to commander.
	return menu_commander->~OnItemSelection(this, item);
}

// Called when an item has been selected (right mouse button).
public func OnItemSelectionAlt(object item)
{
	if (!menu_commander)
		return;
	// Forward to commander.
	return menu_commander->~OnItemSelectionAlt(this, item);
}

// Called when an object is dragged onto the menu
public func MouseDrop(int plr, obj)
{
	// Check if the owners match.
	if (plr != GetOwner()) return false;
		
	// Check if item belongs to a menu.
	if (!obj->~GetMenu()) return false;	
	
	// Check if we allow drag&drop.
	if (!IsDragDropMenu()) return false;
	
	// Forward command to commander.
	return menu_commander->~OnItemDropped(this, obj, nil);
}

// Called when another item has been dropped on an item in this menu.
public func OnItemDropped(object drop_item, object on_item)
{
	if (!menu_commander) return;
	
	// Forward to commander.
	return menu_commander->~OnItemDropped(this, drop_item, on_item);
}

// Called after an item from this menu has been dragged onto an item
public func OnItemDragDone(object drag_item, object on_item)
{
	if (!menu_commander) return;
	
	// Forward to commander.
	return menu_commander->~OnItemDragDone(this, drag_item, on_item);
}

// Called if the mouse cursor enters hovering over an item.
public func OnMouseOverItem(object over_item, object dragged_item)
{
	if (!menu_commander) return;
		
	// Forward to commander.
	return menu_commander->~OnMouseOverItem(this, over_item, dragged_item);
}

// Called if the mouse cursor exits hovering over an item.
public func OnMouseOutItem(object out_item, object dragged_item)
{
	if (!menu_commander) return;
		
	// Forward to commander.
	return menu_commander->~OnMouseOutItem(this, out_item, dragged_item);
}
