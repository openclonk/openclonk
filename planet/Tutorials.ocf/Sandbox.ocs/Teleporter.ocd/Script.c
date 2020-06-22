/**
	Teleporter
	
	@author K-Pone
*/

local Name = "$Name$";
local Description = "$Description$";


public func ControlUse(object clonk, int x, int y)
{
	var gx = clonk->GetX() + x;
	var gy = clonk->GetY() + y;
	
	if (gx < 0 || gx >= LandscapeWidth() || gy < 0 || gy >= LandscapeHeight())
	{
		clonk->Sound("UI::Error");
		return true;
	}
	clonk->SetPosition(gx, gy);
	clonk->Fireworks();
	clonk->Sound("Warp");
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