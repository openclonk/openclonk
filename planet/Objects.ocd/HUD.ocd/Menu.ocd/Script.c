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

static const MENU_Radius = 160;


protected func Construction()
{
	menu_items = [];
	menu_shown = false;
	dragdrop = true;
	// Parallaxity
	this.Parallaxity = [0, 0];
	return;
}

public func IsDragDropMenu() { return dragdrop; }

public func SetDragDropMenu(bool dragdrop)
{
	this.dragdrop = dragdrop;
}

// Sets the commander of this menu.
public func SetCommander(object commander)
{
	menu_commander = commander;
	return;
}

// Sets the menu object for this menu.
public func SetMenuObject(object menuobject)
{
	menu_object = menuobject;
	menuobject->SetMenu(this);
	return;
}

/* Not used currently
func SetMenuIcon(id symbol)
{
	this.Visibility = VIS_Owner;
	if(!symbol)
	{
		SetGraphics(nil,nil,0);
		SetGraphics(nil,nil,1);
	}
	else
	{
		SetGraphics(nil,symbol,1,GFXOV_MODE_IngamePicture);
		SetObjDrawTransform(2000,0,0,0,2000,0,1);
		SetObjDrawTransform(2000,0,0,0,2000,0,0);
	}
}*/

/** Adds an item to this menu.
	@param symbol used to specify the symbol of the menu item, either an id or object.
	@param pos position in the menu where the item should be placed, \c nil for first open slot.
	@param amount the amount displayed next to the symbol.
	@return a pointer to the menu item created, or \c nil if failed.
*/
public func AddItem(symbol, int pos, int amount)
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
		return;
	
	// Create new menu item.
	var item = CreateObject(GUI_MenuItem);
	var index = GetLength(menu_items);
	item->SetSymbol(symbol);
	item->SetAmount(amount);
	menu_items[index] = item;
	item.Visibility = VIS_None;
	if (menu_shown)
		item.Visibility = VIS_Owner;
	UpdateMenu();
	return item;
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
	
	// Trivial case.
	if (n == 1 && total == 1)
		return [0, 0];

	// Packing 7 or less circles.
	if (total <= 7)
	{
		if (n == 1)
			return [0, 0];
		else
		{	
			var x = Cos(60 * (n-1), 2 * MENU_Radius / 3);
			var y = -Sin(60 * (n-1), 2 * MENU_Radius / 3);
			return [x, y];
		}
	}
	
	// Packing 19 or less circles.
	if (total <= 19)
	{
		if (n == 1)
			return [0, 0];
		else if (n <= 7)
		{	
			var x = Cos(60 * (n-1), 2 * MENU_Radius / 5);
			var y = -Sin(60 * (n-1), 2 * MENU_Radius / 5);
			return [x, y];
		}
		else
		{	// TODO: Fix current bad approximation.
			var x = Cos(30 * (n-7) + 15, 4 * MENU_Radius / 5);
			var y = -Sin(30 * (n-7) + 15, 4 * MENU_Radius / 5);
			return [x, y];
		}		
	}
	
	// More cases are not covered yet.
	return;
}

// Gives the radius for an item.
private func GetItemRadius(int total)
{
	if (total == 1)
		return MENU_Radius;
	if (total <= 7)
		return MENU_Radius / 3;
	if (total <= 19)
		return MENU_Radius / 5;	
	return 1;
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
			item->SetPosition(x + pos[0], y + pos[1]);
			item->SetSize(2000*GetItemRadius(item_count));
		}
	}
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
	
	// Check if we allow drag&drop
	if (!IsDragDropMenu()) return false;
	
	// Forward command to commander.
	return menu_commander->OnItemDropped(this, obj, nil);
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
