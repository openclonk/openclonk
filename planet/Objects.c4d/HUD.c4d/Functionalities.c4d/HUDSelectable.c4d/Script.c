
local selector;

public func SetSelector(object sel)
{
	selector = sel;
}

public func GetSelector()
{
	return selector;
}

public func HUDSelectable()
{
	return true;
}

// bootstrap the hud
protected func Recruitment(int iPlr)
{
	if(!FindObject(Find_ID(HUDC),Find_Owner(iPlr)))
		CreateObject(HUDC,10,10,iPlr);

}

// calls to the crew selector hud
protected func OnPromotion() { if(selector) selector->UpdateRank(); }
protected func OnEnergyChange() { if(selector) selector->UpdateHealthBar(); }
protected func OnBreathChange() { if(selector) selector->UpdateBreathBar(); }
protected func OnMagicEnergyChange() { if(selector) selector->UpdateMagicBar(); }
protected func CrewSelection(bool unselect) { if(selector) selector->UpdateSelectionStatus(); }
protected func OnPhysicalChange(string physical, int change, int mode)
{

	if(!selector) return;
	
	// all physicals are resetted
	if(!physical)
	{
		selector->UpdateHealthBar();
		selector->UpdateBreathBar();
		selector->UpdateMagicBar();
	}
	else if(physical == "Energy") selector->UpdateHealthBar();
	else if(physical == "Breath") selector->UpdateBreathBar();
	else if(physical == "Magic") selector->UpdateMagicBar();
}