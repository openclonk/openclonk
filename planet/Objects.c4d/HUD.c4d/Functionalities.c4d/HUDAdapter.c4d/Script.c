/*
	HUD Adapter
	Author: Newton
	
	Clonk-side scripts for the HUD. This object basically redirects the
	engine callbacks for the clonk to the HUD. All crew members that
	are to be shown in the HUD have to include this object and return
	_inherited(); if they overload one of the callbacks used here.

	Requires the ClonkControl.c4d to be included in the clonk too.
*/

local HUDselector, HUDcontroller;

public func SetSelector(object sel) { HUDselector = sel; }
public func GetSelector()           { return HUDselector; }

public func HUDAdapter()
{
	return true;
}

/* Engine callbacks */

// bootstrap the hud
protected func Recruitment(int iPlr)
{
	HUDcontroller = FindObject(Find_ID(HUDC),Find_Owner(iPlr));
	if(!HUDcontroller)
		HUDcontroller = CreateObject(HUDC,10,10,iPlr);
	return _inherited(...); 
}

// calls to the crew selector hud
protected func OnPromotion() { if(HUDselector) HUDselector->UpdateRank(); return _inherited(...); }
protected func OnEnergyChange() { if(HUDselector) HUDselector->UpdateHealthBar(); return _inherited(...); }
protected func OnBreathChange() { if(HUDselector) HUDselector->UpdateBreathBar(); return _inherited(...); }
protected func OnMagicEnergyChange() { if(HUDselector) HUDselector->UpdateMagicBar(); return _inherited(...); }

protected func OnPhysicalChange(string physical, int change, int mode)
{

	if(HUDselector)
	{
		
		// all physicals are resetted
		if(!physical)
		{
			HUDselector->UpdateHealthBar();
			HUDselector->UpdateBreathBar();
			HUDselector->UpdateMagicBar();
		}
		else if(physical == "Energy") HUDselector->UpdateHealthBar();
		else if(physical == "Breath") HUDselector->UpdateBreathBar();
		else if(physical == "Magic") HUDselector->UpdateMagicBar();
	}
	return _inherited(...); 
}

// calls to both crew selector and controller

protected func CrewSelection(bool unselect)
{
	if(HUDselector) HUDselector->UpdateSelectionStatus();
	if(HUDcontroller) HUDcontroller->OnCrewSelection(this,unselect);
	return _inherited(...); 
}

// call from ClonkControl.c4d (self)
public func OnSelectionChanged(int old, int new)
{
	// update selection status in hud
	if(HUDcontroller) HUDcontroller->OnSelectionChanged(old, new);
	return _inherited(...); 
}

// calls to controller

protected func Collection2(object obj)
{

}

protected func Ejection(object obj)
{

}