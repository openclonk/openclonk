/**
	Menu
	Contains all the functions used in all Menus.

	@author boni
*/

local menu_commander; // Object controlling the menu, commands are passed to this object.
local menu_object; // Object for which the menu is shown.
local menu_items; // List of the items in the menu.
local menu_shown; // if the menu is visible right now
local menu_isdragdrop;

/** This function takes care of positioning the objects.
    Has to be implemented by every menu
*/
public func UpdateMenu()
{
	Log("Warning: UpdateMenu has not been implemented for %s!", GetName());
}

protected func Construction()
{
	menu_items = [];
	menu_shown = false;
	menu_isdragdrop = true;
	// Visibility
	this.Visibility = VIS_None;
	// Parallaxity
	this.Parallaxity = [0, 0];
	// Mouse drag image
	this.MouseDragImage = nil;
	this.MouseDrag = MD_NoClick | MD_DropTarget;
	return;
}

/** Returns whether drag and drop is enabled for this menu */
public func IsDragDropMenu() { return menu_isdragdrop; }

/** Sets whether drag and drop is enabled for this menu */
public func SetDragDropMenu(bool is_dragdrop)
{
	menu_isdragdrop = is_dragdrop;
	// Mouse drag image
	this.MouseDragImage = this;
	return;	
}

/** Sets the commander object of this menu. */
public func SetCommander(object commander)
{
	menu_commander = commander;
	return;
}

/** Returns the commander object of this menu. */
public func GetCommander()
{
	return menu_commander;
}

/** Sets the object for which the menu is shown. */
public func SetMenuObject(object menuobject)
{
	menu_object = menuobject;
	return;
}

/** Returns the menu object for this menu. **/
public func GetMenuObject()
{
	return menu_object;
}

/** Adds an item to this menu.
	@param item used to specify the symbol of the menu item, either an id or object.
	@param pos position in the menu where the item should be placed, \c nil for first open slot.
	@return true if successful, false if the position is already taken.
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
	{
		item.Visibility = VIS_Owner;
		UpdateMenu();
	}
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

/** Returns the amount of menu-items in the menu.
	@return the count.
*/
public func GetItemCount()
{
	return GetLength(menu_items);
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
	var length = GetLength(menu_items);
	for(var i = 0; i < length; i++)
	{
		if (menu_items[i] == item)
		{
			menu_items[i]->RemoveObject();
			break;
		}	
	}
	// close gap
	for(; i < length-1; i++)
		menu_items[i] = menu_items[i+1];
	
	SetLength(menu_items, length-1);

	UpdateMenu();
	return;
}

/** Removes all items from the menu. */
public func Clear()
{
	for (var mitem in menu_items)
	{
			mitem->RemoveObject();	
	}
	menu_items = [];
	UpdateMenu();
	return;
}


/* Callbacks from the menu items, to be forwarded to the commander. */

/** Called when an item has been selected (left mouse button). **/
public func OnItemSelection(object item)
{
	if (!menu_commander)
		return;
	// Forward to commander.
	return menu_commander->~OnItemSelection(this, item);
}

/** Called when an item has been selected (right mouse button). **/
public func OnItemSelectionAlt(object item)
{
	if (!menu_commander)
		return;
	// Forward to commander.
	return menu_commander->~OnItemSelectionAlt(this, item);
}

/** Called when an object is dragged onto the menu **/
public func OnMouseDrop(int plr, obj)
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

/** Shows the menu. */
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

/** Hides the menu. */
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

public func Close() 
{
	if(menu_object)
		menu_object->~MenuClosed(this);
	RemoveObject();
}

// Engine callback: if the menu is destroyed, the items must follow.
protected func Destruction()
{
	for (var i = 0; i < GetLength(menu_items); i++)
		if (menu_items[i])
			menu_items[i]->RemoveObject();
	return;
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
