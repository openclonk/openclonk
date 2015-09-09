/**
	HUD Adapter
		
	Clonk-side scripts for the HUD. This object basically redirects the
	engine callbacks for the clonk to the HUD. All crew members that
	are to be shown in the HUD have to include this object and return
	_inherited(); if they overload one of the callbacks used here.

	This adapter redirects to the per player HUD controller and also
	directly to the per clonk HUD selector.

	Requires the ClonkControl.ocd to be included in the clonk too.

	@authors Newton
*/

local HUDcontroller;


public func IsHUDAdapter()
{
	return true;
}


/*-- Engine callbacks --*/

// Bootstrap the HUD on the recruitement of a crew member.
protected func Recruitment(int plr)
{
	HUDcontroller = FindObject(Find_ID(GUI_Controller), Find_Owner(plr));
	if (!HUDcontroller)
		HUDcontroller = CreateObject(GUI_Controller, 0, 0, plr);
	HUDcontroller->~OnCrewRecruitment(this, plr, ...);
	HUDcontroller->~ScheduleUpdateInventory();
	return _inherited(plr, ...);
}

public func SetHUDController(object controller)
{
	// Called from HUD controller when it reinitializes
	HUDcontroller = controller;
	return true;
}

protected func DeRecruitment(int plr)
{
	if (HUDcontroller) 
		HUDcontroller->~OnCrewDeRecruitment(this, plr, ...);
	return _inherited(plr, ...);
}

protected func Death(int killed_by)
{
	if (HUDcontroller) 
		HUDcontroller->~OnCrewDeath(this, killed_by, ...);
	return _inherited(killed_by, ...);
}

protected func Destruction()
{
	if (HUDcontroller) 
		HUDcontroller->~OnCrewDestruction(this, ...);
	return _inherited(...);
}

public func ControlHotkey(int hotindex)
{
	if (HUDcontroller)
		return HUDcontroller->~ControlHotkey(hotindex);
}


public func OnDisplayInfoMessage()
{
	
}

protected func OnPromotion()
{
	if (HUDcontroller)
		HUDcontroller->~OnCrewRankChange(this);
	return _inherited(...); 
}

protected func OnEnergyChange()	
{
	if (HUDcontroller)
		HUDcontroller->~OnCrewHealthChange(this);
	return _inherited(...);

}
protected func OnBreathChange() 
{
	if (HUDcontroller)
		HUDcontroller->~OnCrewBreathChange(this);
	return _inherited(...);
}

protected func OnNameChanged()
{
	if (HUDcontroller)
		HUDcontroller->~OnCrewNameChange(this);
	return _inherited(...);
}

protected func OnPhysicalChange(string physical, int change)
{
	if (HUDcontroller)
	{
		if (!physical)
		{
			HUDcontroller->~OnCrewHealthChange(this);
			HUDcontroller->~OnCrewBreathChange(this);
		}
		else if (physical == "Energy") HUDcontroller->~OnCrewHealthChange(this);
		else if (physical == "Breath") HUDcontroller->~OnCrewBreathChange(this);
	}
	return _inherited(physical, change, ...);
}

// calls to both crew selector and controller
protected func CrewSelection(bool unselect)
{
	if (HUDcontroller)
		HUDcontroller->~OnCrewSelection(this, unselect);
	return _inherited(unselect, ...);
}

// calls to controller
protected func OnCrewEnabled()
{
	if (HUDcontroller)
		HUDcontroller->~OnCrewEnabled(this);
	return _inherited(...);
}

protected func OnCrewDisabled()
{
	if (HUDcontroller)
		HUDcontroller->~OnCrewDisabled(this);
	return _inherited(...);
}

// from ClonkControl.ocd
protected func OnSlotFull(int slot)
{
	if (HUDcontroller)
		HUDcontroller->~OnSlotObjectChanged(slot);
	return _inherited(slot, ...);
}

protected func OnSlotEmpty(int slot)
{
	if (HUDcontroller)
		HUDcontroller->~OnSlotObjectChanged(slot);
	return _inherited(slot, ...);
}

protected func OnHandSelectionChange(int old, int new, int handslot)
{
	if (HUDcontroller)
		HUDcontroller->~OnHandSelectionChange(old, new, handslot);
	return _inherited(old, new, handslot, ...);
}

// handled by GUI_Controller_ActionBars
func StartInteractionCheck(object clonk)
{
	if (HUDcontroller)
		return HUDcontroller->~StartInteractionCheck(clonk, ...);
}

// handled by GUI_Controller_ActionBars
func StopInteractionCheck()
{
	if (HUDcontroller)
		return HUDcontroller->~StopInteractionCheck(...);
}

protected func OnInventoryHotkeyPress(int slot)
{
	if (HUDcontroller)
		HUDcontroller->~OnInventoryHotkeyPress(slot);
	return _inherited(slot, ...);
}

protected func OnInventoryHotkeyRelease(int slot)
{
	if (HUDcontroller)
		HUDcontroller->~OnInventoryHotkeyRelease(slot);
	return _inherited(slot, ...);
}

// when something in the inventory changed
protected func OnInventoryChange()
{
	if (HUDcontroller)
		HUDcontroller->~ScheduleUpdateInventory();
	return _inherited(...);
}

// when a carryheavy object is picked up/dropped
protected func OnCarryHeavyChange(object carried)
{
	if (HUDcontroller)
		if (GetCursor(GetOwner()) == this)
			HUDcontroller->~OnCarryHeavyChange(carried);
	return _inherited(carried, ...);
}

public func Collection2()
{
	if (HUDcontroller)
		HUDcontroller->~ScheduleUpdateInventory();
	return _inherited(...);
}

public func Ejection()
{
	if (HUDcontroller)
		HUDcontroller->~ScheduleUpdateInventory();
	return _inherited(...);
}

public func ControlContents()
{
	if (HUDcontroller)
		HUDcontroller->~ScheduleUpdateInventory();
	return _inherited(...);
}
