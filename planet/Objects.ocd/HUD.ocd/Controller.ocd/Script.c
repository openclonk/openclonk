/**
	HUD Controller
	Controls the player HUD and all its subsystems, which are:
		* Inventory
		* Actionbar
		* Crew selectors
		* Goal
		* Wealth
	All of the subsystems are handled by different included definitions
	and use overloaded functions.
	
	Creates and removes the crew selectors as well as reorders them and
	manages when a crew changes it's controller. Responsible for taking
	care of the action bar.
		
	@authors Newton, Mimmo_O, Zapper, Maikel
*/


// Include the different subsystems of the HUD. They all handle their part
// themselves via overloading of callbacks.
#include GUI_Controller_InventoryBar
#include GUI_Controller_ActionBar
#include GUI_Controller_CrewBar
#include GUI_Controller_Wealth
#include GUI_Controller_Goal


protected func Construction()
{
	// Ensure object is not close to the bottom right border, so sub objects won't be created outside the landscape.
	SetPosition(0, 0);
	// Ensure existing clonks are registered into HUD
	for (var i = 0, crew; crew = GetCrew(GetOwner(), i); ++i)
		if (crew->~IsHUDAdapter())
			crew->SetHUDController(this);
	return _inherited(...);
}

/*-- Destruction --*/

// Remove all HUD-Objects
protected func Destruction()
{
	return _inherited(...);
}


/*-- Callbacks --*/

public func OnCrewRecruitment(object clonk, int plr)
{	
	return _inherited(clonk, plr, ...);
}

public func OnCrewDeRecruitment(object clonk, int plr)
{
	OnCrewDisabled(clonk);
	return _inherited(clonk, plr, ...);
}

public func OnCrewDeath(object clonk, int killer)
{
	OnCrewDisabled(clonk);
	return _inherited(clonk, killer, ...);
}

public func OnCrewDestruction(object clonk)
{
	if(clonk->GetController() != GetOwner()) return _inherited(clonk, ...);
	if(!(clonk->~IsHUDAdapter())) return _inherited(clonk, ...);
	
	OnCrewDisabled(clonk);
	return _inherited(clonk, ...);
}


// called from engine on player eliminated
public func RemovePlayer(int plr, int team)
{
	// at this point, we can assume that all crewmembers have been
	// removed already. Whats left to do is to remove this object,
	// the lower hud and the upper right hud
	// (which are handled by Destruction()
	if (plr == GetOwner()) RemoveObject();
}

/* When loading a savegame, make sure the GUI still works */
func OnSynchronized()
{
	ScheduleCall(this, "Reset", 1);
}

public func Reset()
{
	// The simple way: A full UI reset \o/
	if (GetType(this) == C4V_C4Object)
	{
		// Object call: Reset on owned player
		Destruction();
		Construction();
		
		if(GetCursor(GetOwner()))
			OnCrewSelection(GetCursor(GetOwner()));
	}
	else
	{
		// Definition call: Reset for all players
		RemoveAll(Find_ID(GUI_Controller));
		var plr;
		for (var i=0; i<GetPlayerCount(C4PT_User); ++i)
		{
			var plr = GetPlayerByIndex(i, C4PT_User), cursor;
			CreateObject(GUI_Controller, 0,0, plr);
			if (cursor = GetCursor(plr)) OnCrewSelection(cursor);
		}
	}
	return true;
}
