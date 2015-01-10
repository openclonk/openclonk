/*
	Cotton
	Author: Clonkonaut

	For planting cotton plants or to get cloth
*/

/*public func ControlUse(object clonk, int x, int y, bool box)
{
	if(clonk->GetAction() != "Walk") return true;
	// Search for ground
	x = 0; y = 0;
	if (GBackSemiSolid(x,y)) return true;
	var i = 0;
	while (!GBackSolid(x,y) && i < 15) { ++y; ++i; }
	if (!GBackSolid(x,y)) return true;
	if (GetMaterialVal("Soil", "Material", GetMaterial(x,y)) == 1)
	{
		// Plant!
		clonk->DoKneel();
		CreateObjectAbove(Wheat, x, y, clonk->GetOwner())->SetCon(1);
		RemoveObject();
	}
	else
		clonk->Message("$NoSuitableGround$");

	return true;
}*/

protected func Hit()
{
	Sound("GeneralHit?");
}

public func IsLoomIngredient() { return true; }

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local Rebuy = true;
local Plane = 460;
