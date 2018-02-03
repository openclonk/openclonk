/**
	Devils's Hand
	A tool to remove objects.
	
	@author K-Pone
*/


local Name = "$Name$";
local Description = "$Description$";

public func ControlUse(object clonk, int x, int y)
{
	var remobj = clonk->FindObject(Find_Distance(5, x, y), Find_NoContainer(), Find_Not(Find_ID(Clonk)));
	if (!remobj)
	{
		Sound("UI::Error");
		return;
	}
	remobj->RemoveObject();
	Sound("UI::Click", true, nil, clonk->GetOwner());
	return true;
}

public func QueryRejectDeparture(object clonk)
{
	return true;
}

public func Departure(object clonk)
{
	RemoveObject();
	return;
}