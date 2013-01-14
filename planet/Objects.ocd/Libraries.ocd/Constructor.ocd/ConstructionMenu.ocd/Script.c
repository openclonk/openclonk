/**
	Construction Menu, which consists of a circle of structures which can be
	drawn into the landscape and a display which shows the production costs.
	
	@author Maikel
*/

#include GUI_CircleMenu

local constructinfo_shown;

/** Creates a consruction menu for the calling object. This is supposed to be a crew member, 
	controlled by a player.
	@param producer the producer for which to create the production menu.
	@return a pointer to the created menu, or \c nil if failed.
*/
global func CreateConstructionMenu(object constructor, bool create_at_mouse_pos)
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
	
	if(create_at_mouse_pos)
	{
		var xy = GetPlayerCursorPos(constructor->GetOwner());
		if(xy)
			controller->SetPosition(xy[0],xy[1],true);
	}
	
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
	SetGraphics("BG", GUI_CircleMenu);
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

public func Show()
{
	// Change look
	SetGraphics(nil, Library_ProductionMenu);
	return _inherited(...);
}

private func ShowConstructionInfo(object item)
{
	// TODO: Implement this completely (maybe based on a structure library) and new graphics.
	// For now just show the construction costs somewhere.
	var structure_id = item->GetSymbol();
	var cost_msg = "@";
	var comp, index = 0;
	
	// show energy production/consumption (if any)
	if(structure_id->~IsPowerConsumer())
		cost_msg = Format("%s {{%i}}", cost_msg, Library_PowerConsumer);
	if(structure_id->~IsPowerProducer())
		cost_msg = Format("%s {{%i}}", cost_msg, Library_PowerProducer);
	
	cost_msg = Format("%s         |",cost_msg);
	
	while (comp = GetComponent(nil, index++, nil, structure_id))
		cost_msg = Format("%s %dx {{%i}}", cost_msg, GetComponent(comp, nil, nil, structure_id), comp);
	CustomMessage(cost_msg, this, GetOwner(), 250, 250, nil, nil, nil, 1|2);
	constructinfo_shown = item;
	
	
	
	// show big picture
	SetGraphics(nil, structure_id, 1, GFXOV_MODE_IngamePicture);
	SetObjDrawTransform(400, 0, 270000, 0, 400, -50000, 1);
	
	return;
}

public func HideConstructionInfo()
{
	CustomMessage("", this, GetOwner());
	SetGraphics(nil, nil, 1);
	constructinfo_shown = false;
}

/* Menu properties */

public func IsConstructionMenu() { return true; }

public func HasCommander(object producer)
{
	if (menu_commander == producer)
		return true;
	return false;
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
		//if (menu_commander->~CreateConstructionSite(menu_object, item->GetSymbol()))
		if (menu_commander->~ShowConstructionPreview(menu_object, item->GetSymbol()))
		{
			Close();
			return true;
		}	
	}
	return _inherited(item, ...);
}

// Called when an object is dragged onto the menu
public func OnMouseDrop(int plr, obj)
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

// Called if the mouse cursor enters hovering over an item.
public func OnMouseOverItem(object over_item, object dragged_item)
{
	ShowConstructionInfo(over_item);
	
	return _inherited(over_item, dragged_item, ...);
}

// Called if the mouse cursor exits hovering over an item.
public func OnMouseOutItem(object out_item, object dragged_item)
{
	HideConstructionInfo();
	
	return _inherited(out_item, dragged_item, ...);
}
