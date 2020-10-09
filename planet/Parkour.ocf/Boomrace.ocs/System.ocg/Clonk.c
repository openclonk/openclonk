// The clonk can only hold one item and only collect boompacks.

#appendto Clonk

protected func RejectCollect(id objid, object obj)
{
	if (objid != Boompack) 
		return true;
	return _inherited(objid, obj);
}

local MaxContentsCount = 0;

public func ObjectControl(int plr, int ctrl)
{
	if (IsThrowControl(ctrl))
		return;
	if (IsDropControl(ctrl))
		return;
	
	return _inherited(plr, ctrl, ...);
}

// no extra interactions in this scenario. like dropping carry-heavy objects. :(
public func GetExtraInteractions()
{
	return nil;
}