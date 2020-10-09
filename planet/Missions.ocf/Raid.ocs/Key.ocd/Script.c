/* Key */

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";

func ControlUseStart(object clonk, int ix, int iy)
{
	// Auto-use on spin wheel
	var wheel = FindObject(Find_AtRect(-5,-5, 10, 10), Find_ID(SpinWheel));
	if (!wheel) wheel = FindObject(Find_AtPoint(ix, iy), Find_ID(SpinWheel));
	if (!wheel)
	{
		if (clonk->GetMenu()) clonk->CloseMenu();
		else Dialogue->MessageBox("$KeyNoLock$", clonk, clonk, nil, true);
	}
	else
		wheel->CheckLock(clonk); // using on wheel
	return true;
}

func Hit()
{
	Sound("Hits::Materials::Glass::GlassHit?");
	return true;
}
