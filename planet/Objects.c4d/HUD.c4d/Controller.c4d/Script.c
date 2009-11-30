#strict 2

/*
	Per-Player Controller (HUD)
	Author: Newton

	Creates and removes the crew selectors as well as reorders them and
	manages when a crew changes it's controller. Responsible for taking
	care of the action (inventory) bar.
*/

// TODO - 
// following callbacks missing:
// OnClonkUnRecruitment - clonk gets de-recruited from a crew
// ...? - entire player is eliminated

local actionbar;

protected func Construction()
{
	actionbar = CreateArray();

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
}

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
}

protected func OnClonkDeath(object clonk, int killer)
{
	if(clonk->GetController() != GetOwner()) return;
	
	if(!(clonk->HUDAdapter())) return;
	
	OnCrewDisabled(clonk);
}

public func OnCrewDisabled(object clonk)
{
	// notify the hud and reorder
	clonk->GetSelector()->CrewGone();
	ReorderCrewSelectors();
}

public func OnCrewEnabled(object clonk)
{
	CreateSelectorFor(clonk);
	ReorderCrewSelectors();
}

// call from HUDAdapter (Clonk)
public func OnCrewSelection(object obj, bool deselect)
{

	// selected
	if(!deselect)
	{
		// if several clonks were selected:
		// only the cursor is of interest
		var cursor = GetCursor(GetOwner());
		//Log("cursor: %s",cursor->GetName());
		//if(obj != cursor) return;
		// TODO: what if two clonks are selected? Which clonk gets the actionbar?
		
		// fill actionbar
		
		// inventory
		var i;
		for(i = 0; i < obj->MaxContentsCount(); ++i)
		{
			ActionButton(obj,i);
			actionbar[i]->SetObject(obj->GetItem(i),ACTIONTYPE_INVENTORY,i);
			actionbar[i]->UpdateSelectionStatus();
		}
		// make rest invisible
		for(; i < GetLength(actionbar); ++i)
		{
			// we don't have to remove them all the time, no?
			if(actionbar[i])
				actionbar[i]["Visibility"] = VIS_None;
		}
		
		// and start effect to monitor vehicles and structures...
		// TODO
	}
	else
	{
		// TODO
	}
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
	var obj = GetCursor(GetOwner())->GetItem(slot);
	actionbar[slot]->SetObject(obj, ACTIONTYPE_INVENTORY, slot);
}

private func ActionButton(object forClonk, int i)
{
	var size = ACBT->GetDefWidth();
	var spacing = 12 + size;

	// don't forget the spacings between inventory - vehicle,structure
	var extra = 0;
	if(forClonk->MaxContentsCount() <= i) extra = 80;
	
	var bt = actionbar[i];
	// no object yet... create it
	if(!bt)
	{
		bt = CreateObject(ACBT,0,0,GetOwner());
	}
	
	bt->SetPosition(64 + i * spacing + extra, -16 - size/2);
	
	if(i+1 == 10) bt->SetHotkey(0);
	else if(i+1 < 10) bt->SetHotkey(i+1);
	
	bt->SetCrew(forClonk);
	
	actionbar[i] = bt;
}

private func CreateSelectorFor(object clonk)
{
		var selector = CreateObject(CSLR,10,10,-1);
		selector->SetCrew(clonk);
		clonk->SetSelector(selector);
		return selector;
}

public func ReorderCrewSelectors()
{
	// somehow new crew gets sorted at the beginning
	// because we dont want that, the for loop starts from the end
	var j = 0;
	for(var i=GetCrewCount(GetOwner())-1; i >= 0; --i)
	{
		var spacing = 12;
		var crew = GetCrew(GetOwner(),i);
		var sel = crew->GetSelector();
		if(sel)
		{
			sel->SetPosition(32 + j * (CSLR->GetDefWidth() + spacing) + CSLR->GetDefWidth()/2, 16+CSLR->GetDefHeight()/2);
			if(j+1 == 10) sel->SetHotkey(0);
			else if(j+1 < 10) sel->SetHotkey(j+1);
		}
		
		j++;
	}
}
