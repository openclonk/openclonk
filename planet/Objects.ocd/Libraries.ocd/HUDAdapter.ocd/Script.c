/**
	HUD Adapter
		
	Clonk-side scripts for the HUD. This object basically redirects the
	engine callbacks for the clonk to the HUD. All crew members that
	are to be shown in the HUD have to include this object and return
	_inherited(...); if they overload one of the callbacks used here.

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

// Either returns the current HUD controller or creates one.
// But only if owner is a human otherwise returns nil.
private func GetHUDController()
{
	var plr = GetOwner();
	// During runtime join, plr isn't a valid player in the joining client yet
	// when this function is called from OnSynchronized(). This code previously
	// checked player validity before returning a cached HUD object which would
	// cause a desync.
	if (HUDcontroller) return HUDcontroller;
	if (GetPlayerType(plr) != C4PT_User) return nil;
	var controllerDef = Library_HUDController->GetGUIControllerID();
	HUDcontroller = FindObject(Find_ID(controllerDef), Find_Owner(plr));
	if (!HUDcontroller)
		HUDcontroller = CreateObject(controllerDef, AbsX(0), AbsY(0), plr);
	return HUDcontroller;
}

// Update HUD controller e.g. when it was reinitialized
public func SetHUDController(object new_controller)
{
	HUDcontroller = new_controller;
	return true;
}

/*-- Engine callbacks --*/

// Bootstrap the HUD on the recruitement of a crew member.
private func Recruitment(int plr)
{
	if (GetHUDController())
	{
		HUDcontroller->~OnCrewRecruitment(this, plr, ...);
		HUDcontroller->~ScheduleUpdateInventory();
	}
	return _inherited(plr, ...);
}

// On savegame load or after section change, ensure that there's a HUD adapter
public func OnSynchronized(...)
{
	if (GetHUDController())
		HUDcontroller->~ScheduleUpdateInventory();
	return _inherited(...);
}

private func DeRecruitment(int plr)
{
	if (HUDcontroller)
		HUDcontroller->~OnCrewDeRecruitment(this, plr, ...);
	return _inherited(plr, ...);
}

private func Death(int killed_by)
{
	if (HUDcontroller)
		HUDcontroller->~OnCrewDeath(this, killed_by, ...);
	return _inherited(killed_by, ...);
}

private func Destruction()
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

private func OnPromotion()
{
	if (HUDcontroller)
		HUDcontroller->~OnCrewRankChange(this);
	return _inherited(...); 
}

private func OnEnergyChange(int change, int cause, int caused_by)
{
	if (HUDcontroller)
		HUDcontroller->~OnCrewHealthChange(this, change, cause, caused_by);
	return _inherited(change, cause, caused_by, ...);

}
private func OnBreathChange(int change)
{
	if (HUDcontroller)
		HUDcontroller->~OnCrewBreathChange(this, change);
	return _inherited(...);
}

private func OnMagicEnergyChange(int change)
{
	if (HUDcontroller)
		HUDcontroller->~OnCrewMagicChange(this, change);
	return _inherited(...);
}

private func OnNameChanged()
{
	if (HUDcontroller)
		HUDcontroller->~OnCrewNameChange(this);
	return _inherited(...);
}

private func OnPhysicalChange(string physical, int change)
{
	if (HUDcontroller)
	{
		if (!physical)
		{
			HUDcontroller->~OnCrewHealthChange(this, change);
			HUDcontroller->~OnCrewBreathChange(this, change);
		}
		else if (physical == "Energy") HUDcontroller->~OnCrewHealthChange(this, change);
		else if (physical == "Breath") HUDcontroller->~OnCrewBreathChange(this, change);
	}
	return _inherited(physical, change, ...);
}

private func CrewSelection(bool unselect)
{
	if (HUDcontroller)
		HUDcontroller->~OnCrewSelection(this, unselect);
	return _inherited(unselect, ...);
}

private func OnCrewEnabled()
{
	if (HUDcontroller)
		HUDcontroller->~OnCrewEnabled(this);
	return _inherited(...);
}

private func OnCrewDisabled()
{
	if (HUDcontroller)
		HUDcontroller->~OnCrewDisabled(this);
	return _inherited(...);
}

// from ClonkControl.ocd
private func OnSlotFull(int slot)
{
	if (HUDcontroller)
		HUDcontroller->~OnSlotObjectChanged(slot);
	return _inherited(slot, ...);
}

private func OnSlotEmpty(int slot)
{
	if (HUDcontroller)
		HUDcontroller->~OnSlotObjectChanged(slot);
	return _inherited(slot, ...);
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

private func OnInventoryHotkeyPress(int slot)
{
	if (HUDcontroller)
		HUDcontroller->~OnInventoryHotkeyPress(slot);
	return _inherited(slot, ...);
}

private func OnInventoryHotkeyRelease(int slot)
{
	if (HUDcontroller)
		HUDcontroller->~OnInventoryHotkeyRelease(slot);
	return _inherited(slot, ...);
}

// when something in the inventory changed
private func OnInventoryChange()
{
	if (HUDcontroller)
		HUDcontroller->~ScheduleUpdateInventory();
	return _inherited(...);
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
