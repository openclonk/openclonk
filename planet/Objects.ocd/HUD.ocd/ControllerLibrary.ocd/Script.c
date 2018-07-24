/**
	HUD Controller
	Controls the player HUD and all its subsystems. In order to make a
	new HUD include this library and the desired subsystems.

	All of the subsystems are handled by different included definitions
	and use overloaded functions.

	Receives all engine callbacks that are forwarded to environment objects.

	Creates and removes the crew selectors as well as reorders them and
	manages when a crew changes it's controller. Responsible for taking
	care of the action bar.

	@authors Newton, Mimmo_O, Zapper, Maikel
*/

// All elements visible to HUD owner only - so NO_OWNER on sub-elements can be used to make them invisible temporarily.
local Visibility = VIS_Owner;

// Is created by Recruitment from the HUD adapter.
private func Construction()
{
	SetPosition(0, 0);
	// Ensure existing clonks are registered into HUD
	for (var i = 0, crew; crew = GetCrew(GetOwner(), i); ++i)
		if (crew->~IsHUDAdapter())
			crew->SetHUDController(this);
	return _inherited(...);
}

/** Should always remove all objects associated with the HUD
	private func Destruction() {}
*/


/*-- Callbacks --*/


/** The following callbacks are forwarded by the HUD adapter:

	OnClonkRecruitment(object clonk, int plr)
	OnClonkDeRecruitment(object clonk, int plr)
	OnCrewDeath(object clonk, int killed_by)
	OnCrewDestruction(object clonk)
	ControlHotKey(int hotkey)
	OnCrewRankChange(object clonk)
	OnCrewHealthChange(object clonk, int change, int cause, int caused_by)
	OnCrewBreathChange(object clonk, int change)
	OnCrewMagicChange(object clonk, int change)
	OnCrewNameChange(object clonk)
	OnCrewSelection(object clonk, bool unselect)
	OnCrewEnabled(object clonk)
	OnCrewDisabled(object clonk)
	OnSlotObjectChange(int slot)
	StartInteractionCheck(object clonk)
	StopInteractionCheck()
	OnInventoryHotkeyPress(int slot)
	OnInventoryHotkeyRelease(int slot)
	ScheduleUpdateInventory()
*/


public func RemovePlayer(int plr, int team)
{
	// at this point, we can assume that all crewmembers have been
	// removed already. Whats left to do is to remove this object,
	// the lower hud and the upper right hud
	// (which are handled by Destruction()
	if (plr == GetOwner()) RemoveObject();
}

/* When loading a savegame, make sure the GUI still works */
public func OnSynchronized()
{
	ScheduleCall(this, this.Reset, 1);
}

public func Reset()
{
	// The simple way: A full UI reset \o/
	if (GetType(this) == C4V_C4Object)
	{
		// Object call: Reset on owned player
		this->~Destruction();
		Construction();
		
		if(GetCursor(GetOwner()))
			this->~OnCrewSelection(GetCursor(GetOwner()));
	}
	else
	{
		// Definition call: Reset for all players
		RemoveAll(Find_ID(GetGUIControllerID()));
		for (var i=0; i<GetPlayerCount(C4PT_User); ++i)
		{
			var plr = GetPlayerByIndex(i, C4PT_User);
			var controller = CreateObject(GetGUIControllerID(), 0, 0, plr);
			var cursor = GetCursor(plr); 
			if (cursor)
				controller->~OnCrewSelection(cursor);
		}
	}
	return true;
}

func GetGUIControllerID()
{
	return GUI_Controller;
}
