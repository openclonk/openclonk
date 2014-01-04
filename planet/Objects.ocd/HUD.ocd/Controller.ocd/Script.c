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

// Local variables containing the GUI-Elements
local actionbar;	// Array, action-buttons at the bottom
local inventory;	// Array, inventory-buttons at the left side
local markers;		// Array, the gui-markers.
local carryheavy;	// Object, optional inventory-button only shown when clonk is carrying a carry-heavy object
local wealth;		// Object, displays wealth of the player

local progress_bar_links;

static const GUI_MAX_ACTIONBAR = 10; // maximum amount of actionbar-slots

protected func Construction()
{
	actionbar = [];
	markers = [];
	inventory = [];
	progress_bar_links = [];
	
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
	
	
	// wealth display
	wealth = CreateObject(GUI_Wealth,0,0,GetOwner());
	wealth->SetPosition(-16-GUI_Wealth->GetDefHeight()/2,8+GUI_Wealth->GetDefHeight()/2);
	wealth->Update();
	
	// inventory display
	MakeInventory();
}

/* Destruction */

// Remove all HUD-Objects
protected func Destruction()
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

	if(inventory)
		for(var i=0; i<GetLength(inventory); ++i)
		{
			if(inventory[i])
				inventory[i]->RemoveObject();
		}
	
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
		
	var crew = FindObjects(Find_ID(GUI_CrewSelector), Find_Owner(GetOwner()));
	for(var o in crew)
		o->RemoveObject();
}

/* Inventory-Bar stuff */

// Updates the Inventory in 1 frame
private func ScheduleUpdateInventory()
{
	ScheduleCall(this, "UpdateInventory", 1, 0);
}

private func MakeInventory()
{
	// distance between slots
	var d = 72;
	// upper barrier
	var y = 200;

	// and the carry heavy slot
	var bt = CreateObject(GUI_Backpack_Slot_Icon,0,0,GetOwner());
	bt->SetHUDController(this);
	bt->SetPosition(40+d, y);
	bt->SetSlotId(-1);
	bt.Visibility = VIS_None;
	
	carryheavy = bt;
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


/* Markers */

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

/* Inventory stuff */
func UpdateInventory()
{
	// only display if we have a clonk
	var c = GetCursor(GetOwner());
	if(!c) return 1;
	
	// sort out old progress bars
	if(GetLength(progress_bar_links))
	{
		var old_progress_bar_links = progress_bar_links[:];
		progress_bar_links = [];
		
		for(var bar in old_progress_bar_links)
		{
			if(!bar.effect) continue;
			PushBack(progress_bar_links, bar);
		}
	}
	
	// update inventory-slots
	for(var i=0; i<GetLength(inventory); i++)
	{
		var item = c->GetItem(inventory[i]->GetSlotId());
		inventory[i]->SetSymbol(item);
		inventory[i]->SetUnselected();
		
		inventory[i]->ClearProgressBarLink();
		// re-add progress bar if possible
		for(var bar in progress_bar_links)
		{
			if(bar.obj != item) continue;
			inventory[i]->SetProgressBarLink(bar.effect);
			break;
		}
	}
	
	// update hand-indicator
	if(c->~IsCarryingHeavy())
	{
		carryheavy->SetSelected(-1);
	}
	else
		for(var i=0; i < c->~HandObjects(); ++i)
		{
			var handpos = c->GetHandItemPos(i);
			if(inventory[handpos]) 
				inventory[handpos]->SetSelected(i);
		}
}	

// sets the link of the progress bar for a certain slot
// the link is an effect that has the properties "max" and "current"
func SetProgressBarLinkForObject(object what, proplist e)
{
	PushBack(progress_bar_links, {obj = what, effect = e});
	ScheduleUpdateInventory();
}

// Shows the Carryheavy-Inventoryslot if obj is set
// Removes it if it's nil
func OnCarryHeavyChange(object obj)
{
	if (!carryheavy) return; // safety if called during construction/destruction process
	carryheavy->SetSymbol(obj);

	if(obj == nil)
	{
		carryheavy->SetUnselected();
		carryheavy.Visibility = VIS_None;
	}
	else
		this.Visibility = VIS_Owner;
	
	UpdateInventory();
}

/* Hotkey Control */

// executes the mouseclick onto an actionbutton through hotkeys
public func ControlHotkey(int hotindex)
{
	// button exists?
	if(!actionbar[hotindex]) return false;
	
	// button is assigned to a clonk?
    var clonk = actionbar[hotindex]->GetCrew();
   	if(!clonk) return false;
   	
   	// press the button
  	actionbar[hotindex]->~MouseSelection(GetOwner());
  	
   	return true;
}

/** Callbacks **/

// insert new clonk into crew-selectors on recruitment
public func OnCrewRecruitment(object clonk, int plr)
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
	ScheduleUpdateInventory();
}

public func OnCrewDeRecruitment(object clonk, int plr)
{
	// not my business
	if(plr != GetOwner()) return;
	if(!(clonk->HUDAdapter())) return;
	
	OnCrewDisabled(clonk);
}

public func OnCrewDeath(object clonk, int killer)
{
	if(clonk->GetController() != GetOwner()) return;
	if(!(clonk->~HUDAdapter())) return;
	
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
	// not my business
	if(plr != GetOwner()) return;
	
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
	
	// update
	ScheduleUpdateInventory();
}

public func OnCrewEnabled(object clonk)
{
	if (!clonk->GetSelector()) CreateSelectorFor(clonk);
	ReorderCrewSelectors();
}


// call from HUDAdapter (Clonk)
public func OnCrewSelection(object clonk, bool deselect)
{
	// selected
	if(!deselect)
	{
		// and start effect to monitor vehicles and structures...
		AddEffect("IntSearchInteractionObjects",clonk,1,10,this,nil,0);
		
		// clear inventory buttons
		UpdateInventoryButtons(clonk);
	}
	else
	{
		// remove effect
		RemoveEffect("IntSearchInteractionObjects",clonk);
	}
	
	// clear actionbuttons
	ClearActionButtons();
		
	// update
	ScheduleUpdateInventory();
	OnCarryHeavyChange(clonk->~GetCarryHeavy());
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

// takes care of displaying the interactions
public func FxIntSearchInteractionObjectsTimer(object target, effect, int time)
{

	// find vehicles & structures & script interactables
	var startAt = effect.startAt;
	var i = startAt;
	
	//var hotkey = i+1-target->HandObjects();
	var hotkey = i+1;
	
	// Get custom interactions from the clonk
	// extra interactions are an array of proplists. proplists have to contain at least a function pointer "f", a description "desc" and an "icon" definition/object. Optional "front"-boolean for sorting in before/after other interactions.
	var extra_interactions = target->~GetExtraInteractions()??[]; // if not present, just use []. Less error prone than having multiple if(!foo).
	// we could sort the interactions, but there usually will only be like 1-3. Not really worth it.
	
	// Add buttons:
	
	// all except structures only if outside
	if(!target->Contained())
	{
		// add extra-interactions Priority 0
		for(var interaction in extra_interactions)
			if(interaction.Priority == 0)
				ActionButton(target, i++, interaction.Object, ACTIONTYPE_EXTRA, hotkey++, nil, interaction);
	
		// add interactables (script interface)
		var interactables = FindObjects(Find_AtPoint(target->GetX()-GetX(),target->GetY()-GetY()),Find_Func("IsInteractable",target),Find_NoContainer(), Find_Layer(target->GetObjectLayer()));
		var j, icnt;
		for(var interactable in interactables)
		{
			icnt = interactable->~GetInteractionCount();
			if(!icnt)
				icnt = 1;

			for(j=0; j < icnt; j++)
			{
				ActionButton(target,i++,interactable,ACTIONTYPE_SCRIPT,hotkey++, j);
			}
		}
		
		// add extra-interactions Priority 1
		for(var interaction in extra_interactions)
			if(interaction.Priority == 1)
				ActionButton(target, i++, interaction.Object, ACTIONTYPE_EXTRA, hotkey++, nil, interaction);
		
		// add vehicles
		var vehicles = FindObjects(Find_AtPoint(target->GetX()-GetX(),target->GetY()-GetY()),Find_OCF(OCF_Grab),Find_NoContainer(), Find_Layer(target->GetObjectLayer()));
		for(var vehicle in vehicles)
		{
			ActionButton(target,i++,vehicle,ACTIONTYPE_VEHICLE,hotkey++);
		}
		
		// add extra-interactions Priority 2
		for(var interaction in extra_interactions)
			if(interaction.Priority == 2)
				ActionButton(target, i++, interaction.Object, ACTIONTYPE_EXTRA, hotkey++, nil, interaction);
	}

	// add structures
	var structures = FindObjects(Find_AtPoint(target->GetX()-GetX(),target->GetY()-GetY()),Find_OCF(OCF_Entrance),Find_NoContainer(), Find_Layer(target->GetObjectLayer()));
	for(var structure in structures)
	{
		ActionButton(target,i++,structure,ACTIONTYPE_STRUCTURE,hotkey++);
	}
	
	// add extra-interactions after everything
	for(var interaction in extra_interactions)
		if(interaction.Priority == nil || interaction.Priority > 2)
			ActionButton(target, i++, interaction.Object, ACTIONTYPE_EXTRA, hotkey++, nil, interaction);
	
	ClearActionButtons(i);
	
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

// call from HUDAdapter or inventory-buttons
public func OnHandSelectionChange(int old, int new, int handslot)
{
	if(inventory[old])
		inventory[old]->SetUnselected();
	if(inventory[new])
		inventory[new]->SetSelected(handslot);
	
	OnSlotObjectChanged(handslot);
}

protected func OnInventoryHotkeyPress(int slot)
{
	if(inventory[slot])
		inventory[slot]->OnMouseOver(GetOwner());
}

protected func OnInventoryHotkeyRelease(int slot)
{
	if(inventory[slot])
		inventory[slot]->OnMouseOut(GetOwner());
}

// call from HUDAdapter (Clonk)
public func OnSlotObjectChanged(int slot)
{	
	// refresh inventory
	ScheduleUpdateInventory();
}

/** Helper Functions **/

// Insert an inventory slot into the inventory-bar
private func InventoryButton()
{
// distance between slots
	var d = 72;
	// upper barrier
	var y = 200;

	// index
	var i = GetLength(inventory);
	
	// create inventory slots
	var bt = CreateObject(GUI_Backpack_Slot_Icon,0,0,GetOwner());
	bt->SetHUDController(this);
	bt->SetPosition(40, y + d*i);
	bt->SetSlotId(i);
		
	CustomMessage(Format("@%d.", i+1), bt, nil, -40, 54);
	inventory[i] = bt;
	
	return bt;
}

// sets the inventory size to the currently selected clonk
private func UpdateInventoryButtons(object clonk)
{
	if(!clonk) return;
	
	var size = clonk->~MaxContentsCount();
	
	// need to create more inventory buttons?
	if(size > GetLength(inventory))
		for(var i=0; i < size; ++i)
			if(!inventory[i])
				InventoryButton();
	
	// need to remove some inventory buttons?
	if(size < GetLength(inventory))
		for(i=GetLength(inventory)-1; i >= size; i--)
			inventory[i]->RemoveObject();
	
	SetLength(inventory, size);
}

// Insert a button into the actionbar at pos
private func ActionButton(object forClonk, int pos, object interaction, int actiontype, int hotkey, int num, proplist extra)
{
	// the actionbar has a maximum size
	if(pos >= GUI_MAX_ACTIONBAR)
		return nil;

	var spacing = 100;
	
	var bt = actionbar[pos];
	
	// no object yet... create it
	if(!bt)
	{
		bt = CreateObject(GUI_ObjectSelector,0,0,GetOwner());
	}

	bt->SetPosition(401 + pos * spacing, -45);
	
	bt->SetCrew(forClonk);
	bt->SetObject(interaction,actiontype,pos,hotkey, num, extra);
	
	actionbar[pos] = bt;
	return bt;
}

/** Removes all actionbar buttons after start */
private func ClearActionButtons(int start)
{

	// make rest invisible
	for(var j = start; j < GetLength(actionbar); ++j)
	{
		// we don't have to remove them all the time, no?
		if(actionbar[j])
			actionbar[j]->Disable();
	}
}

/** Returns how many actionbar-buttons are actually visible */
private func GetRealActionbarLength()
{
	var i=0;
	for(var j = 0; j < GetLength(actionbar); ++j)
		if(actionbar[j]->ShowsItem())	i++;
	return i;
}

/** Removes all messages that the actionbuttons show */
private func ClearButtonMessages()
{
	for(var i = 0; i < GetLength(actionbar); ++i)
	{
		if(actionbar[i])
			actionbar[i]->ClearMessage();
	}
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
