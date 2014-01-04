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

local HUDselector, HUDcontroller;

public func SetSelector(object sel) { HUDselector = sel; }
public func GetSelector() { return HUDselector; }

public func HUDAdapter()
{
	return true;
}

// hotkey control
public func ControlHotkey(int hotindex)
{
	if (HUDcontroller)
		return HUDcontroller->ControlHotkey(hotindex);
}

/* Engine callbacks */

// bootstrap the hud
protected func Recruitment(int plr)
{
	HUDcontroller = FindObject(Find_ID(GUI_Controller), Find_Owner(plr));
	if (!HUDcontroller)
		HUDcontroller = CreateObject(GUI_Controller, 10, 10, plr);
	
	HUDcontroller->OnCrewRecruitment(this, plr, ...);
	HUDcontroller->ScheduleUpdateInventory();
	
	return _inherited(plr, ...);
}

protected func DeRecruitment(int plr)
{
	if (HUDcontroller) HUDcontroller->OnCrewDeRecruitment(this, plr, ...);
	
	return _inherited(plr, ...);
}

protected func Death(int killed_by)
{
	if (HUDcontroller) HUDcontroller->OnCrewDeath(this, killed_by, ...);

	return _inherited(killed_by,...);
}

protected func Destruction()
{
	if (HUDcontroller) HUDcontroller->OnCrewDestruction(this, ...);

	return _inherited(...);
}

public func OnDisplayInfoMessage()
{
	if (HUDcontroller)
		HUDcontroller->ClearButtonMessages();
}
	
// calls to the crew selector hud
protected func OnPromotion()
{
	if (HUDselector)
		HUDselector->UpdateRank();
	return _inherited(...); 
}

protected func OnEnergyChange()	
{
	if (HUDselector)
		HUDselector->UpdateHealthBar();
	return _inherited(...);

}
protected func OnBreathChange() {
	if (HUDselector)
		HUDselector->UpdateBreathBar();
	return _inherited(...);
}

protected func OnMagicEnergyChange() {
	return _inherited(...);
}

protected func OnNameChanged()
{
	if (HUDselector)
		HUDselector->UpdateName();
	return _inherited(...);
}

protected func OnPhysicalChange(string physical, int change)
{
	if (HUDselector)
	{		
		// all physicals are resetted
		if (!physical)
		{
			HUDselector->UpdateHealthBar();
			HUDselector->UpdateBreathBar();
			HUDselector->UpdateMagicBar();
		}
		else if (physical == "Energy") HUDselector->UpdateHealthBar();
		else if (physical == "Breath") HUDselector->UpdateBreathBar();
		else if (physical == "Magic") HUDselector->UpdateMagicBar();
	}
	return _inherited(physical, change, ...);
}

// calls to both crew selector and controller
protected func CrewSelection(bool unselect)
{
	if (HUDselector)
		HUDselector->UpdateSelectionStatus();
	if (HUDcontroller)
		HUDcontroller->OnCrewSelection(this,unselect);
	return _inherited(unselect, ...);
}

// calls to controller
protected func OnCrewEnabled()
{
	if (HUDcontroller)
		HUDcontroller->OnCrewEnabled(this);
	return _inherited(...);
}

protected func OnCrewDisabled()
{
	if (HUDcontroller)
		HUDcontroller->OnCrewDisabled(this);
	return _inherited(...);
}

// from ClonkControl.ocd
protected func OnSlotFull(int slot)
{
	if (HUDcontroller)
		HUDcontroller->OnSlotObjectChanged(slot);
		
	return _inherited(slot, ...);
}

protected func OnSlotEmpty(int slot)
{
	if (HUDcontroller)
		HUDcontroller->OnSlotObjectChanged(slot);
	
	return _inherited(slot, ...);
}

// used to add a progress bar to an inventory slot
// "effect" refers to an effect with the properties "max" and "current" that is used to keep the progress bar state up-to-date
func SetProgressBarLinkForObject(object what, proplist effect)
{
	if(HUDcontroller)
		HUDcontroller->SetProgressBarLinkForObject(what, effect);
	return _inherited(what, effect, ...);
}

protected func OnHandSelectionChange(int old, int new, int handslot)
{
	if (HUDcontroller)
		HUDcontroller->OnHandSelectionChange(old, new, handslot);
	return _inherited(old, new, handslot, ...);
}

protected func OnInventoryHotkeyPress(int slot)
{
	if (HUDcontroller)
		HUDcontroller->OnInventoryHotkeyPress(slot);
	return _inherited(slot, ...);
}

protected func OnInventoryHotkeyRelease(int slot)
{
	if (HUDcontroller)
		HUDcontroller->OnInventoryHotkeyRelease(slot);
	return _inherited(slot, ...);
}

// when something in the inventory changed
protected func OnInventoryChange()
{
	if (HUDcontroller)
		HUDcontroller->ScheduleUpdateInventory();
	return _inherited(...);
}

// when a carryheavy object is picked up/dropped
protected func OnCarryHeavyChange(object carried)
{
	if(HUDcontroller)
		if(GetCursor(GetOwner()) == this)
			HUDcontroller->OnCarryHeavyChange(carried);
		
	return _inherited(carried, ...);
}

func Collection2()
{
	if (HUDcontroller)
		HUDcontroller->ScheduleUpdateInventory();
	return _inherited(...);
}

func Ejection()
{
	if (HUDcontroller)
		HUDcontroller->ScheduleUpdateInventory();
	return _inherited(...);
}

func ControlContents()
{
	if (HUDcontroller)
		HUDcontroller->ScheduleUpdateInventory();
	return _inherited(...);
}
