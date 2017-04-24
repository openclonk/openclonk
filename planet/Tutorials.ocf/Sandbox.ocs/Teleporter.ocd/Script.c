
/*

	Teleporter
	
	@Author: K-Pone

*/

local Name = "$Name$";
local Description = "$Description$";

func Initialize()
{
	
}

func ControlUse(object clonk, x, y)
{
	var gx, gy;
	
	gx = clonk->GetX() + x;
	gy = clonk->GetY() + y;
	
	if (gx < 0 || gx >= LandscapeWidth() || gy < 0 || gy >= LandscapeHeight())
	{
		clonk->Sound("UI::Error");
		return;
	}
	
	clonk->SetPosition(gx, gy);
	clonk->Fireworks();
	clonk->Sound("warp");
}