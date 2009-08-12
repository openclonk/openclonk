#strict 2

/*
	Per-Player Controller

	Creates and removes the crew selectors as well as reorders them and
	manages when a crew changes it's controller	
*/


// following callbacks missing:
// OnClonkUnRecruitment - clonk gets de-recruited from a crew

protected func Construction()
{
	// find all clonks of this crew which do not have a selector yet (and can have one)
	for(var i=GetCrewCount(GetOwner())-1; i >= 0; --i)
	{
		var crew = GetCrew(GetOwner(),i);
		if(!(crew->HUDSelectable())) continue;
		
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
	
	if(!(clonk->HUDSelectable())) return;
	
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
	if(GetController(clonk) != GetOwner()) return;
	
	if(!(clonk->HUDSelectable())) return;
	
	// notify the hud
	clonk->GetSelector()->CrewGone();
	
	// and reorder
	ReorderCrewSelectors();
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
		var crew = GetCrew(GetOwner(),i);
		var sel = crew->GetSelector();
		if(sel)
		{
			sel->SetPosition(32 + j * (GetDefWidth(CSLR) + spacing) + GetDefWidth(CSLR)/2, 16+GetDefHeight(CSLR)/2);
			if(j+1 == 10) sel->SetHotkey(0);
			else sel->SetHotkey(j+1);
		}
		
		var spacing = 12;
		j++;
	}
}