/*--- Metal Chunk ---*/

#include Library_CarryHeavy

public func GetCarryMode(clonk) { return CARRY_BothHands; }
public func GetCarryPhase() { return 800; }

protected func Hit()
{
	Sound("GeneralHit?");
}

public func IsChunk() { return true; }
public func IsFoundryProduct() { return true; }
public func GetFuelNeed() { return 100; }

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(30,0,0,1),Trans_Rotate(-30,1,0,0),Trans_Scale(1300)),def);
}

local Name = "$Name$";
local Description = "$Description$";
local Rebuy = true;
local Touchable = 2;
local Plane = 470;