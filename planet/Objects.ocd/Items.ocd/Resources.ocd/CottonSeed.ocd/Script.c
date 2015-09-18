/*
	Cotton Seed
	Author: Clonkonaut

	For planting cotton plants or to get cloth
*/

#include Library_Seed

local lib_seed_plant = Cotton;

public func GetCarryMode() { return CARRY_HandBack; }
public func GetCarryBone() { return "main"; }

private func Hit()
{
	Sound("GeneralHit?");
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Rebuy = true;
local Plane = 460;
local BlastIncinerate = 5;
local ContactIncinerate = 2;