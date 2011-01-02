/*-- Axe --*/

private func Hit()
{
	Sound("RockHit");
	return 1;
}

public func ControlUse(object pByClonk, int iX, int iY)
{
	var tree = FindObject(Find_AtPoint(iX, iY), Find_Func("IsTree"), Find_Func("IsStanding"),
	           Find_Layer(GetObjectLayer()), Find_NoContainer());
	if( tree )
	{
		pByClonk->SetAction("Chop"); //FIXME: actually implement this
		Sound("KnightConfirm*");
	} else {
		if(pByClonk->GetAction() == "Chop")
			pByClonk->SetAction("Idle");
	}
	return 1;
}

public func IsTool() { return 1; }

public func IsToolProduct() { return 1; }

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
