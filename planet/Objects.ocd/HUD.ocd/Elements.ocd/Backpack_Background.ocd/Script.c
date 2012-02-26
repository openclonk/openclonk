/**
	Backpack_Background
	The background of the backpack area. Also doubles as hover-area for showing the backpack-slots.

	@author boni
*/

local controller;

public func SetHUDController(object c) { controller = c; }

public func Initialize()
{
	// Visibility
	this.Visibility = VIS_Owner;
	// Parallaxity
	this.Parallaxity = [0, 0];
	
	SetName("");
}

public func OnMouseOver(int plr)
{
	if(!controller || GetOwner() == NO_OWNER)
		return nil;
	
	controller->ShowInventory();
	controller->ScheduleHideInventory();
}