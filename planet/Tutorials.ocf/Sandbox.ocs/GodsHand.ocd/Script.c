
/*

	God's Hand, a tool to create objects.
	
	@author: K-Pone

*/


local Name = "$Name$";
local Description = "$Description$";

func Initialize()
{
	
}

func ControlUse(object clonk, x, y)
{
	var objdef = clonk.ObjectSpawnDefinition;
	
	
	if (objdef == Marker)
	{
		var marker = clonk->PlaceNewMarker();
		if (marker) marker->SetPosition(clonk->GetX() + x, clonk->GetY() + y);
	}
	else
	{
		clonk->CreateObject(objdef, x, y);
	}
	
	Sound("UI::Click", true, nil, clonk->GetOwner());
}