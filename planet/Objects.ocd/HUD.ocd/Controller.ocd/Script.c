/**
	HUD Controller

	Controls the player HUD and all its subsystems, which are:
		* Inventory (=)
		* Actionbar
		* Crew selectors
		* Goal
		* Wealth
		
	Creates and removes the crew selectors as well as reorders them and
	manages when a crew changes it's controller. Responsible for taking
	care of the action bar.
		
	@authors Newton, Mimmo_O
*/

#include GUI_Controller_InventoryBar
#include GUI_Controller_ActionBar

// Local variables containing the GUI-Elements
local wealth;		// Object, displays wealth of the player




protected func Construction()
{
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
	
	
	// wealth display
	wealth = CreateObject(GUI_Wealth,0,0,GetOwner());
	wealth->SetPosition(-16-GUI_Wealth->GetDefHeight()/2,8+GUI_Wealth->GetDefHeight()/2);
	wealth->Update();
	
	return _inherited();
}

/* Destruction */

// Remove all HUD-Objects
protected func Destruction()
{
	// remove all hud objects that are managed by this object
	if(wealth)
		wealth->RemoveObject();
	
	var HUDgoal = FindObject(Find_ID(GUI_Goal),Find_Owner(GetOwner()));
	if(HUDgoal)
		HUDgoal->RemoveObject();
		
	var crew = FindObjects(Find_ID(GUI_CrewSelector), Find_Owner(GetOwner()));
	for(var o in crew)
		o->RemoveObject();
		
	return _inherited();
}


/*-- Wealth --*/

protected func OnWealthChanged(int plr)
{
	if (plr != GetOwner()) 
		return;
	if (wealth) 
		wealth->Update();
	return;
}

/*-- Goal --*/

public func OnGoalUpdate(object goal)
{
	var HUDgoal = FindObject(Find_ID(GUI_Goal), Find_Owner(GetOwner()));
	if (!goal)
	{
		if (HUDgoal) HUDgoal->RemoveObject();
	}
	else
	{
		if (!HUDgoal)
		{
			HUDgoal = CreateObject(GUI_Goal, 0, 0, GetOwner());
			HUDgoal->SetPosition(-64-16-GUI_Goal->GetDefHeight()/2,8+GUI_Goal->GetDefHeight()/2);
		}
		HUDgoal->SetGoal(goal);
	}
}





/** Callbacks **/

// insert new clonk into crew-selectors on recruitment
protected func OnClonkRecruitment(object clonk, int plr)
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

protected func OnClonkDeRecruitment(object clonk, int plr)
{
	OnCrewDisabled(clonk);
}

protected func OnClonkDeath(object clonk, int killer)
{
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
	CreateSelectorFor(clonk);
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
protected func UpdateTransferZone()
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
