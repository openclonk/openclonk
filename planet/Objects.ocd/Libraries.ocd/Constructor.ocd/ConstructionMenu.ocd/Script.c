/**
	Construction Menu, which consists of a circle of structures which can be
	drawn into the landscape and a display which shows the production costs.
	
	@author Maikel
*/

#include GUI_Menu

local constructinfo_shown;

/** Creates a consruction menu for the calling object. This is supposed to be a crew member, 
	controlled by a player.
	@param producer the producer for which to create the production menu.
	@return a pointer to the created menu, or \c nil if failed.
*/
global func CreateConstructionMenu(object constructor)
{
	// Safety checks.
	if (!this) return;
	if (!(GetOCF() & OCF_CrewMember)) return;
	if (!(this->~HasMenuControl())) return;

	// Create the menu controller.
	var controller = CreateObject(Library_ConstructionMenu);
	controller->SetMenuObject(this);
	this->SetMenu(controller);
	controller->SetCommander(constructor);
	
	// Add all possible structures to the menu.
	controller->AddMenuStructures(constructor, this);
	
	// Show and return production menu.
	controller->Show();
	return controller;
}

protected func Construction()
{
	SetPosition(600, 400);
	// Set original menu graphics.
	SetGraphics("BG", GUI_Menu);
	return _inherited(...);
}

public func AddMenuStructures(object constructor, object clonk)
{
	for (var structure in constructor->GetConstructionPlans(clonk->GetOwner()))
	{
		var item = CreateObject(GUI_MenuItem);
		if (!AddItem(item))
			return item->RemoveObject();
		item->SetSymbol(structure);
	}
	return;
}

private func ShowConstructionInfo(object item)
{
	// TODO: Implement this completely (maybe based on a structure library) and new graphics.
	// For now just show the construction costs somewhere.
	var structure_id = item->GetSymbol();
	var cost_msg = "@";
	var comp, index = 0;
	while (comp = GetComponent(nil, index++, nil, structure_id))
		cost_msg = Format("%s %dx {{%i}}", cost_msg, GetComponent(comp, nil, nil, structure_id), comp);
	CustomMessage(cost_msg, this, GetOwner(), 250, 270, nil, nil, nil, 1);
	constructinfo_shown = item;
	return;
}

public func HideConstructionInfo()
{
	CustomMessage("", this, GetOwner());
	constructinfo_shown = false;
}

/* Menu properties */

public func IsProductionMenu() { return true; }
// UpdateCursor is called
public func CursorUpdatesEnabled() { return true; }

public func Close() 
{
	if(menu_object)
		menu_object->~MenuClosed(this);
	RemoveObject();
}

public func HasCommander(object producer)
{
	if (menu_commander == producer)
		return true;
	return false;
}

// Callback if the mouse is moved
public func UpdateCursor(int dx, int dy)
{
	var item = FindObject(Find_AtPoint(dx, dy), Find_ID(GUI_MenuItem));
	if (!item || item->GetMenu() != this)
	{
		if (constructinfo_shown)
			HideConstructionInfo();
		return;
	}
	if (item == constructinfo_shown)
		return;
	ShowConstructionInfo(item);
}

/* Callbacks from the menu items, to be translated into commands for the producer. */

// Called when a click outside the menu has been made.
public func OnMouseClick(int x, int y, bool alt)
{
	// Close menu if not clicked on the menu.
	if (Distance(x, y, 0, 0) > 160)
		Close();
	return;
}

// Called when an item has been selected (left mouse button).
public func OnItemSelection(object item)
{
	if (menu_commander)
	{
		// Close menu if a construction site has been created.
		if (menu_commander->~CreateConstructionSite(menu_object, item->GetSymbol()))
			Close();	
	}
	return _inherited(item, ...);
}

// Called when an object is dragged onto the menu
public func MouseDrop(int plr, obj)
{
	return _inherited(plr, obj, ...);
}

// Called when another item has been dropped on an item in this menu.
public func OnItemDropped(object drop_item, object on_item)
{
	return _inherited(drop_item, on_item, ...);
}

// Called after an item from this menu has been dragged onto an item
public func OnItemDragDone(object drag_item, object on_item)
{
	return _inherited(drag_item, on_item, ...);
}

