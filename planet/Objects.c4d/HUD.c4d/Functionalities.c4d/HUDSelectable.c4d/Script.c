#strict 2
#appendto CLNK

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
		
	return _inherited(...);
}