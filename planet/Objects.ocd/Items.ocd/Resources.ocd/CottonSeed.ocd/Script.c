/*
	Cotton Seed
	Author: Clonkonaut

	For planting cotton plants or to get cloth
*/

#include Library_Seed
#include Library_Flammable

local lib_seed_plant = Cotton;

public func GetCarryMode() { return CARRY_HandBack; }
public func GetCarryBone() { return "main"; }

private func Hit()
{
	Sound("Hits::GeneralHit?");
}

public func IsFuel() { return true; }
public func GetFuelAmount(int requested_amount)
{
    // disregard the parameter, because only a complete chunk should be removed 
	if (this != CottonSeed)	return GetCon()/2;
	return 50;
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Plane = 460;
local BlastIncinerate = 5;
local ContactIncinerate = 2;