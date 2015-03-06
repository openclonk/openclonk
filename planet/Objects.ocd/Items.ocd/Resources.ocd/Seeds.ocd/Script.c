/*
	Seeds
	Author: Clonkonaut

	For planting wheat or to get flour
*/

public func RejectUse(object clonk)
{
	// General inability?
	if(clonk->GetProcedure() != "WALK") return true;
	if (GBackSemiSolid(0, 0)) return true;
	/*
	This check might make sense, so that you can just hold the button and walk over the ground until the soil is fitting.
	However, you would never get the $NoSuitableGround$ message in that case.
	// Check soil below the Clonk's feet
	var ground_y = clonk->GetDefBottom() - clonk->GetY() + 1;
	if (GetMaterialVal("Soil", "Material", GetMaterial(0, ground_y)) == 0) return true;
	*/
	return false;
}

public func ControlUse(object clonk, int x, int y, bool box)
{
	var ground_y = clonk->GetDefBottom() - clonk->GetY() + 1;
	
	if (GetMaterialVal("Soil", "Material", GetMaterial(0, ground_y)) == 1)
	{
		// Plant!
		clonk->DoKneel();
		CreateObjectAbove(Wheat, 0, ground_y, clonk->GetOwner())->SetCon(1);
		RemoveObject();
	}
	else
		clonk->Message("$NoSuitableGround$");

	return true;
}

protected func Hit()
{
	Sound("GeneralHit?");
}

public func IsMillIngredient() { return true; }

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local Rebuy = true;
