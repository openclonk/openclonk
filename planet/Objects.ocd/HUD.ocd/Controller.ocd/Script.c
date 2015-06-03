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
#include GUI_Controller_Wealth
#include GUI_Controller_Goal


protected func Construction()
{
	// ensure object is not close to the bottom right border, so subobjects won't be created outside the landscape
	SetPosition(0,0);
	
	// find all clonks of this crew which do not have a selector yet (and can have one)
	for(var i=0; i < GetCrewCount(GetOwner()); ++i)
	{
		var crew = GetCrew(GetOwner(),i);
		if(!(crew->HUDAdapter())) continue;
		
		var sel = crew->GetSelector();
		if(!sel)
			CreateSelectorFor(crew);
	}
	
	// reorder the crew selectors
	ReorderCrewSelectors();
	
	return _inherited();
}

/* Destruction */

// Remove all HUD-Objects
protected func Destruction()
{
	var crew = FindObjects(Find_ID(GUI_CrewSelector), Find_Owner(GetOwner()));
	for(var o in crew)
		o->RemoveObject();
		
	return _inherited();
}


global func AddHUDMarker(int player, picture, string altpicture, string text, int duration, bool urgent, object inform)
{
	var number = 0;
	var padding = GUI_Marker->GetDefHeight()+5;
	var hud = FindObject(Find_ID(GUI_Controller),Find_Owner(player));
	number = hud->GetFreeMarkerPosition();
	hud.markers[number] = CreateObject(GUI_Marker,0,0,player);
	hud.markers[number] -> SetPosition(5+(GUI_Marker->GetDefWidth()/2),-240-(GUI_Marker->GetDefHeight()/2) - number*padding);
	hud.markers[number] -> SetVisual(picture, altpicture);
	if(inform) hud.markers[number].toInform = inform;
	if(duration) AddEffect("IntRemoveMarker",hud.markers[number],100,duration,hud.markers[number]);
	if(urgent) AddEffect("IntUrgentMarker",hud.markers[number],100,2,hud.markers[number]);
	if(text) hud.markers[number]->SetName(text);
	
	return hud.markers[number];
}


/** Callbacks **/

// insert new clonk into crew-selectors on recruitment
public func OnCrewRecruitment(object clonk, int plr)
{	
	// not enabled
	if(!clonk->GetCrewEnabled()) return;
	
	// if the clonk already has a hud, it means that he belonged to another
	// crew. So we need another handling here in this case.
	var sel;
	if(sel = clonk->GetSelector())
	{
		var owner = sel->GetOwner();
		sel->UpdateController();
		
		// reorder stuff in the other one
		var othercontroller = FindObject(Find_ID(GetID()), Find_Owner(owner));
		othercontroller->ReorderCrewSelectors();
	}
	// create new crew selector
	else
	{
		CreateSelectorFor(clonk);
	}
	
	// reorder the crew selectors
	ReorderCrewSelectors();
	
	return _inherited(clonk, plr, ...);
}

public func OnCrewDeRecruitment(object clonk, int plr)
{
	OnCrewDisabled(clonk);
}

public func OnCrewDeath(object clonk, int killer)
{
	OnCrewDisabled(clonk);
}

public func OnCrewDestruction(object clonk)
{
	if(clonk->GetController() != GetOwner()) return;
	if(!(clonk->~HUDAdapter())) return;
	
	OnCrewDisabled(clonk);
}


// called from engine on player eliminated
public func RemovePlayer(int plr, int team)
{
	// at this point, we can assume that all crewmembers have been
	// removed already. Whats left to do is to remove this object,
	// the lower hud and the upper right hud
	// (which are handled by Destruction()
	return RemoveObject();
}

public func OnCrewDisabled(object clonk)
{
	// notify the hud and reorder
	var selector = clonk->GetSelector();
	if(selector)
	{
		selector->CrewGone();
		ReorderCrewSelectors(clonk);
	}
	return _inherited(clonk, ...);
}

public func OnCrewEnabled(object clonk)
{
	if (!clonk->GetSelector()) CreateSelectorFor(clonk);
	ReorderCrewSelectors();
	
	return _inherited(clonk, ...);
}



/** Creates a crew selector for the given clonk.
    Should be followed by a ReorderCrewSelectors call
*/
private func CreateSelectorFor(object clonk)
{
	var selector = CreateObject(GUI_CrewSelector,10,10,-1);
	selector->SetCrew(clonk);
	clonk->SetSelector(selector);
	return selector;
}


/** Rearranges the CrewSelectors in the correct order */
private func ReorderCrewSelectors(object leaveout)
{
	var j = 0;
	for(var i=0; i < GetCrewCount(GetOwner()); ++i)
	{
		var spacing = 12;
		var crew = GetCrew(GetOwner(),i);
		if(crew == leaveout) continue;
		var sel = crew->GetSelector();
		if(sel)
		{
			sel->SetPosition(60 + 32 + j * (GUI_CrewSelector->GetDefWidth() + spacing) + GUI_CrewSelector->GetDefWidth()/2, 16+GUI_CrewSelector->GetDefHeight()/2);
			if(j+1 == 10) sel->SetHotkey(0);
			else if(j+1 < 10) sel->SetHotkey(j+1);
		}
		
		j++;
	}
}



/* When loading a savegame, make sure the GUI still works */
func OnSynchronized()
{
	ScheduleCall(this, "Reset", 1);
}

private func Reset()
{
	// The simple way: A full UI reset \o/
	Destruction();
	Construction();
	
	if(GetCursor(GetOwner()))
		OnCrewSelection(GetCursor(GetOwner()));
}
