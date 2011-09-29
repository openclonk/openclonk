/**
	HUD Controller

	Controls the player HUD and all its subsystems, which are:
		* Backpack
		* Hand items
		* Energy/Breath tubes
		* Actionbar
		* Crew selectors
		* Goal
		* Wealth
		
	Creates and removes the crew selectors as well as reorders them and
	manages when a crew changes it's controller. Responsible for taking
	care of the action (inventory) bar.
		
	@authors Newton, Mimmo_O
*/

local actionbar;
local wealth;
local deco;
local markers;
local backpack;

// Tubes.
local healthtube;
local breathtube;

protected func Construction()
{
	actionbar = [];
	markers = [];
	backpack = [];
	
	// find all clonks of this crew which do not have a selector yet (and can have one)
	for(var i=GetCrewCount(GetOwner())-1; i >= 0; --i)
	{
		var crew = GetCrew(GetOwner(),i);
		if(!(crew->HUDAdapter())) continue;
		
		var sel = crew->GetSelector();
		if(!sel)
			CreateSelectorFor(crew);
	}
	
	// reorder the crew selectors
	ReorderCrewSelectors();
	
	// background decoration
	deco = CreateObject(GUI_Background,0,0,GetOwner());
	deco->SetPosition(1,-1);
	deco->SetControllerObject(this);
	
	// wealth display
	wealth = CreateObject(GUI_Wealth,0,0,GetOwner());
	wealth->SetPosition(-16-GUI_Wealth->GetDefHeight()/2,8+GUI_Wealth->GetDefHeight()/2);
	wealth->Update();
	
	// breathtube
	MakeBreathTube();
	
	// healthtube
	MakeHealthTube();
	
	// manatube
	// MakeManaTube();
	
	// backpack display
	for(var i=0; i<5; i++)
	{
		var bt = CreateObject(GUI_RingMenu_Icon,0,0,GetOwner());
		bt->SetPosition(74+Sin((i*72)+36,43),-74-Cos((i*72)+36,43));	
		bt->SetExtraData(i+2);
		bt->SetSize(48000);
		//bt->SetObject(this->GetItem(2+i),0,2+i,0);
		bt->SetObjDrawTransform(1,0,0,0,1,0,0);
		backpack[GetLength(backpack)] = bt;
	}
	//AddEffect("UpdateBackpack",this,1,1,nil,nil);		
	

	
}

private func ScheduleUpdateBackpack()
{
	ScheduleCall(this, "UpdateBackpack", 1, 0);
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
		if (!HUDgoal) HUDgoal = CreateObject(GUI_Goal, 0, 0, GetOwner());
		HUDgoal->SetPosition(-64-16-GUI_Goal->GetDefHeight()/2,8+GUI_Goal->GetDefHeight()/2);
		HUDgoal->SetGoal(goal);
	}
}

/*-- Health tube --*/

private func MakeHealthTube()
{	
	var tube = CreateObject(GUI_HealthTube, 0, 0, GetOwner());
	tube->MakeTube();
	tube->Update();
	healthtube = tube;
	return;
}

public func UpdateHealthTube()
{
	if (healthtube)
		healthtube->Update();
	return;
}

/*-- Breath tube --*/

private func MakeBreathTube()
{
	var tube = CreateObject(GUI_BreathTube, 0, 0, GetOwner());
	tube->MakeTube();
	tube->Update();
	breathtube = tube;
	return;
}

public func UpdateBreathTube()
{
	if (breathtube)
		breathtube->Update();
	return;
}

/*
protected func MakeManaTube()
{
	var tube = CreateObject(GUI_ManaTube,0,0,this->GetOwner());
	tube->SetPosition(1,-1);
	tube->SetAction("Swirl");
	var ftube = CreateObject(GUI_ManaTube,0,0,this->GetOwner());
	ftube->SetPosition(1,-1);
	ftube->MakeTop();
	//tube->SetTubes(btube,ftube);
	tube->SetTubes(ftube);
	tube->Update();
	AddEffect("Update",tube,100,1,tube);
	
	tubes[GetLength(tubes)] =  tube;
	tubes[GetLength(tubes)] = ftube;

}
*/


public func GetFreeMarkerPosition()
{	
	for(var i=0; i < GetLength(markers); i++)
	{
		if(!markers[i]) return i;	
	}
	return GetLength(markers);
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

func UpdateBackpack()
{
	var c = GetCursor(GetOwner());
	if(!c) return 1;
	
	for(var i=0; i<GetLength(backpack); i++)
	{
		backpack[i]->SetAmount(0);  
		backpack[i]->SetSymbol(c->GetItem(backpack[i]->GetExtraData())); 
		backpack[i]->SetGraphics(nil,nil,9);
		backpack[i]->SetGraphics(nil,nil,10);
		backpack[i]->SetGraphics(nil,nil,11);
		backpack[i]->SetGraphics(nil,nil,12);
	}
}	

/*
global func FxUpdateBackpackTimer(target) { 
	if(!target) return -1;
	if(!GetCursor(target->GetOwner())) return 1;
	if(!GetCursor(target->GetOwner())->~HUDShowItems())
	{
		for (var i = 0; i < GetLength(target.backpack); i++)
			target.backpack[i]->SetNothing();
		return 1;
	}
	for(var i=0; i<GetLength(target.backpack); i++)
	{
		target.backpack[i]->SetAmount(0);  
		target.backpack[i]->SetSymbol(GetCursor(target->GetOwner())->GetItem(target.backpack[i]->GetExtraData())); 
		target.backpack[i]->SetGraphics(nil,nil,9);
		target.backpack[i]->SetGraphics(nil,nil,10);
		target.backpack[i]->SetGraphics(nil,nil,11);
		target.backpack[i]->SetGraphics(nil,nil,12);
	}
}*/

protected func OnClonkRecruitment(object clonk, int plr)
{
	// not my business
	if(plr != GetOwner()) return;
	
	if(!(clonk->HUDAdapter())) return;
	
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
	
	// update
	ScheduleUpdateBackpack();
	UpdateHealthTube();
	UpdateBreathTube();
}

protected func OnClonkDeRecruitment(object clonk, int plr)
{
	// not my business
	if(plr != GetOwner()) return;
	if(!(clonk->HUDAdapter())) return;
	
	OnCrewDisabled(clonk);
}

protected func OnClonkDeath(object clonk, int killer)
{
	if(clonk->GetController() != GetOwner()) return;
	if(!(clonk->~HUDAdapter())) return;
	
	OnCrewDisabled(clonk);
}



// called from engine on player eliminated
public func RemovePlayer(int plr, int team)
{
	// not my business
	if(plr != GetOwner()) return;
	
	// at this point, we can assume that all crewmembers have been
	// removed already. Whats left to do is to remove this object,
	// the lower hud and the upper right hud
	// (which are handled by Destruction()
	return RemoveObject();
}

public func Destruction()
{
	// remove all hud objects that are managed by this object
	if(wealth)
		wealth->RemoveObject();
	if(actionbar)
	{
		for(var i=0; i<GetLength(actionbar); ++i)
		{
			if(actionbar[i])
				actionbar[i]->RemoveObject();
		}
	}
	
	if(healthtube)
		healthtube->RemoveObject();
		
	if (breathtube)
		breathtube->RemoveObject();
	
	if(markers)
	{
		for(var i=0; i<GetLength(markers); ++i)
		{
			if(markers[i])
				markers[i]->RemoveObject();
		}
	}
	
	var HUDgoal = FindObject(Find_ID(GUI_Goal),Find_Owner(GetOwner()));
	if(HUDgoal)
		HUDgoal->RemoveObject();
	if(deco)
		deco->RemoveObject();
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
	
	// update
	ScheduleUpdateBackpack();
	UpdateHealthTube();
	UpdateBreathTube();
}

public func OnCrewEnabled(object clonk)
{
	CreateSelectorFor(clonk);
	ReorderCrewSelectors();
}

// call from HUDAdapter (Clonk)
public func OnCrewSelection(object clonk, bool deselect)
{
	// selected
	if(!deselect)
	{
		// fill actionbar
		// inventory
		var i;
	//	var deco = CreateObject(GUI_ObjectSelector_Background,0,0,clonk->GetOwner());
	//	deco->SetPosition(337,-38);
		
		for(i = 0; i < Min(2,clonk->HandObjects()); ++i)
		{
			ActionButton(clonk,i,clonk->GetItem(i),ACTIONTYPE_INVENTORY);
		}
		ClearButtons(i);
		
		// and start effect to monitor vehicles and structures...
		AddEffect("IntSearchInteractionObjects",clonk,1,10,this,nil,i);
	}
	else
	{
		// remove effect
		RemoveEffect("IntSearchInteractionObjects",clonk,0);
		ClearButtons();
	}
	
	// update
	ScheduleUpdateBackpack();
	UpdateHealthTube();
	UpdateBreathTube();
}

public func FxIntSearchInteractionObjectsEffect(string newname, object target)
{
	if(newname == "IntSearchInteractionObjects")
		return -1;
}

public func FxIntSearchInteractionObjectsStart(object target, effect, int temp, startAt)
{
	if(temp != 0) return;
	effect.startAt = startAt;
	EffectCall(target,effect,"Timer",target,effect,0);
}

public func FxIntSearchInteractionObjectsTimer(object target, effect, int time)
{

	// find vehicles & structures & script interactables
	var startAt = effect.startAt;
	var i = startAt;
	
	var vehicles = CreateArray();
	var pushed = nil;
	
	var exclusive = false;
	var hotkey = i+1-target->HandObjects();
	
	if((!target->Contained()))
	{
		vehicles = FindObjects(Find_AtPoint(target->GetX()-GetX(),target->GetY()-GetY()),Find_OCF(OCF_Grab),Find_NoContainer(), Find_Layer(target->GetObjectLayer()));
		
		// don't forget the vehicle that the clonk is pushing (might not be found
		// by the findobjects because it is not at that point)
		if(target->GetProcedure() == "PUSH" && (pushed = target->GetActionTarget()))
		{
			// if the pushed vehicle has been found, we can just continue
			var inside = false;
			for(var vehicle in vehicles)
				if(vehicle == pushed)
					inside = true;
			
			// otherwise we must add it before the rest
			if(!inside)
			{
				ActionButton(target,i,pushed,ACTIONTYPE_VEHICLE,hotkey++);
				if(actionbar[i]->Selected()) exclusive = true;
				++i;
			}
		}
	}
	
	// search vehicles
	for(var vehicle in vehicles)
	{
		ActionButton(target,i,vehicle,ACTIONTYPE_VEHICLE,hotkey++);
		if(actionbar[i]->Selected()) exclusive = true;
		++i;
	}

	// search structures
	var structures = FindObjects(Find_AtPoint(target->GetX()-GetX(),target->GetY()-GetY()),Find_OCF(OCF_Entrance),Find_NoContainer(), Find_Layer(target->GetObjectLayer()));
	for(var structure in structures)
	{
		ActionButton(target,i,structure,ACTIONTYPE_STRUCTURE,hotkey++);
		if(actionbar[i]->Selected()) exclusive = true;
		++i;
	}

	// search interactables (script interface)
	var interactables = FindObjects(Find_AtPoint(target->GetX()-GetX(),target->GetY()-GetY()),Find_Func("IsInteractable",target),Find_NoContainer(), Find_Layer(target->GetObjectLayer()));
	for(var interactable in interactables)
	{
		ActionButton(target,i,interactable,ACTIONTYPE_SCRIPT,hotkey++);
		++i;
	}
	
	ClearButtons(i);
	
	return;
}

// call from HUDAdapter (Clonk)
public func OnSelectionChanged(int old, int new)
{
	//Log("selection changed from %d to %d", old, new);
	// update both old and new
	actionbar[old]->UpdateSelectionStatus();
	actionbar[new]->UpdateSelectionStatus();
}

// call from HUDAdapter (Clonk)
public func OnSlotObjectChanged(int slot)
{
	//Log("slot %d changed", slot);
	var cursor = GetCursor(GetOwner());
	if(!cursor) return;
	var obj = cursor->GetItem(slot);
	actionbar[slot]->SetObject(obj, ACTIONTYPE_INVENTORY, slot);
	
	// refresh backpack
	ScheduleUpdateBackpack();
}

private func ActionButton(object forClonk, int pos, object interaction, int actiontype, int hotkey)
{
	//var size = GUI_ObjectSelector->GetDefWidth();
	var spacing = deco.padding;

	// don't forget the spacings between inventory - vehicle,structure
	var extra = 0;
	if(forClonk->HandObjects() <= pos) extra = 80;
	
	var bt = actionbar[pos];
	// no object yet... create it
	if(!bt)
	{
		bt = CreateObject(GUI_ObjectSelector,0,0,GetOwner());
	}

		if(pos==0)
		{ 
			bt->SetGraphics("None");
			bt->SetPosition(288, -48);
		}	
		else if(pos==1) 
		{
			bt->SetPosition(381, -48);
			bt->SetGraphics("None");
		}
		else
		{
			bt->SetCon(90);
			bt->SetPosition(491 + (pos-2) * spacing, -45);
		}

	
	bt->SetCrew(forClonk);
	bt->SetObject(interaction,actiontype,pos,hotkey);
	
	actionbar[pos] = bt;
	return bt;
}

private func ClearButtons(int start)
{

	// make rest invisible
	for(var j = start; j < GetLength(actionbar); ++j)
	{
		// we don't have to remove them all the time, no?
		if(actionbar[j])
			actionbar[j]->Clear();
	}
	//if(deco->GetSlotNumber() != -1)
	//if(deco->GetSlotNumber() != GetRealActionbarLength())
		deco->SlideTo(GetRealActionbarLength());
}

private func GetRealActionbarLength()
{
	var i=0;
	for(var j = 2; j < GetLength(actionbar); ++j)
		if(actionbar[j]->ShowsItem())	i++;
	return i;
}

public func ClearButtonMessages()
{
	for(var i = 0; i < GetLength(actionbar); ++i)
	{
		if(actionbar[i])
			actionbar[i]->ClearMessage();
	}
}

// hotkey control
public func ControlHotkey(int hotindex)
{
	if(!actionbar[hotindex]) return false;
    var clonk = actionbar[hotindex]->GetCrew();
   	if(!clonk) return false;
    hotindex += clonk->HandObjects();
   	if(!actionbar[hotindex]) return false;
   	// only if it is not already used
  	actionbar[hotindex]->~MouseSelection(GetOwner());
   	return true;
}

private func CreateSelectorFor(object clonk)
{
		var selector = CreateObject(GUI_CrewSelector,10,10,-1);
		selector->SetCrew(clonk);
		clonk->SetSelector(selector);
		return selector;
}



public func ReorderCrewSelectors(object leaveout)
{
	// somehow new crew gets sorted at the beginning
	// because we dont want that, the for loop starts from the end
	var j = 0;
	for(var i=GetCrewCount(GetOwner())-1; i >= 0; --i)
	{
		var spacing = 12;
		var crew = GetCrew(GetOwner(),i);
		if(crew == leaveout) continue;
		var sel = crew->GetSelector();
		if(sel)
		{
			sel->SetPosition(32 + j * (GUI_CrewSelector->GetDefWidth() + spacing) + GUI_CrewSelector->GetDefWidth()/2, 16+GUI_CrewSelector->GetDefHeight()/2);
			if(j+1 == 10) sel->SetHotkey(0);
			else if(j+1 < 10) sel->SetHotkey(j+1);
		}
		
		j++;
	}
}

