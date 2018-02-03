/**
	God's Hand
	A tool to create objects.
	
	@author K-Pone
*/


local Name = "$Name$";
local Description = "$Description$";

public func ControlUse(object clonk, int x, int y)
{
	var objdef = clonk.ObjectSpawnDefinition;
	if (objdef == Marker)
	{
		var marker = clonk->PlaceNewMarker();
		if (marker)
			marker->SetPosition(clonk->GetX() + x, clonk->GetY() + y);
	}
	else
	{
		clonk->CreateObject(objdef, x, y);
	}
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