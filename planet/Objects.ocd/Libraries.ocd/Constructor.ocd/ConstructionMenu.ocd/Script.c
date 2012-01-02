/**
	Construction Menu, which consists of a circle of structures which can be
	drawn into the landscape and a display which shows the production costs.
	
	@author Maikel
*/

#include GUI_Menu

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
	SetGraphics(nil, GUI_Menu);
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

/* Menu properties */

public func IsProductionMenu() { return true; }

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

/* Callbacks from the menu items, to be translated into commands for the producer. */

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

// Called when an item has been selected (right mouse button).
public func OnItemSelectionAlt(object item)
{
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

