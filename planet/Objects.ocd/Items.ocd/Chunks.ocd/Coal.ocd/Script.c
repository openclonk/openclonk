/*--- Coal Chunk ---*/

#include Library_CarryHeavy

public func GetCarryMode(clonk) { return CARRY_BothHands; }
public func GetCarryPhase() { return 800; }

protected func Hit(x, y)
{
	StonyObjectHit(x,y);
	return true;
}

public func IsFuel() { return true; }
public func GetFuelAmount() { return 400; }
public func IsChunk() { return true; }

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(30,0,0,1),Trans_Rotate(-30,1,0,0),Trans_Scale(1300)),def);
}

local Name = "$Name$";
local Description = "$Description$";
local Rebuy = true;
local BlastIncinerate = 5;
local ContactIncinerate = 1;
local Touchable = 2;
local Plane = 460;