/*--- Planks ---*/

#include Library_CarryHeavy

public func GetCarryMode(clonk) { return CARRY_BothHands; }
public func GetCarryPhase() { return 800; }

protected func Hit()
{
	Sound("WoodHit?");
}

public func IsFuel() { return true; }
public func GetFuelAmount() { return 150; }
public func IsChunk() { return true; }
public func IsSawmillProduct() { return true; }

local Name = "$Name$";
local Description = "$Description$";
local Rebuy = true;
local BlastIncinerate = 5;
local ContactIncinerate = 1;
local Touchable = 2;
local Plane = 470;