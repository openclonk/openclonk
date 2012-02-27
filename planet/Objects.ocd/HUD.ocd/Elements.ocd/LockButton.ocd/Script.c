/**
	ExpandButton
	Locks/Unlocks the inventory bar.

	@author boni
*/

local Name = "$Name$";
local Description = "$Description$";

local locked;
local controller;

public func SetHUDController(object c) { controller = c; }
public func IsLocked() { return locked; }

public func Initialize()
{
	// Lock-Buttons need an owner for extradata
	var owner = GetOwner();
	if(owner == NO_OWNER)
		return;
	
	// Visibility
	this.Visibility = VIS_Owner;
	// Parallaxity
	this.Parallaxity = [0, 0];
	
	locked = !!GetPlrExtraData(owner, "Inventory_Lock");
	if(!locked)
		SetGraphics("Released", GetID());
}

public func MouseSelection(int plr)
{
	// we need a controller to report back to
	if(!controller)
		return nil;
	if(plr != GetOwner())
		return nil;

	if(locked)
		Unlock();
	else
		Lock();
}

public func Lock()
{
	if(!controller || GetOwner() == NO_OWNER)
		return nil;
	
	locked = true;
	
	// tell controller to do its stuff
	controller->ShowInventory();
	
	// update graphics
	SetGraphics(nil, GetID());
	
	// and save the result.
	SetPlrExtraData(GetOwner(), "Inventory_Lock", true);
}

public func Unlock()
{
	if(!controller || GetOwner() == NO_OWNER)
		return nil;
	
	locked = false;
	
	// tell controller to do its stuff
	controller->ScheduleHideInventory();
	
	// update graphics
	SetGraphics("Released", GetID());
		
	SetPlrExtraData(GetOwner(), "Inventory_Lock", false);
}
