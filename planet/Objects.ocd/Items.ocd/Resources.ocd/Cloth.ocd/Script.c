/*-- Cloth --*/

#include Library_CarryHeavy

public func GetCarryMode(clonk) { return CARRY_BothHands; }
public func GetCarryPhase() { return 800; }
public func GetCarryTransform(clonk)
{
	if (GetCarrySpecial(clonk))
		return Trans_Translate(2000, 4500, 6500);
}

private func Hit()
{
	Sound("Hits::GeneralHit?");
}

public func IsLoomProduct() { return true; }

private func Definition(def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(-45, 1), Trans_Rotate(-20, 0, 0, 1)), def);
}

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
local Plane = 470;
local Components = {CottonSeed = 1};