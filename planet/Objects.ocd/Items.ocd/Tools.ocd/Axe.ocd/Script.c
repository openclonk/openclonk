/*-- Axe --*/

private func Hit()
{
	Sound("RockHit");
	return 1;
}

public func GetCarryMode() { return CARRY_HandBack; }
public func GetCarryBone() { return "main"; }
public func GetCarryTransform()
{
	var act = Contained()->GetAction();
	if(act != "Walk" && act != "Jump")
		return Trans_Mul(Trans_Translate(0,4500,0), Trans_Rotate(90,0,1,0), Trans_Rotate(180,0,0,1) );

	return Trans_Rotate(90, 0, 1, 0);
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

public func IsTool() { return true; }
public func IsToolProduct() { return true; }

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Rebuy = true;
