/*-- Fern --*/

#include Library_Plant

local plant_seed_chance = 25;
local plant_seed_area = 120;
local plant_seed_amount = 4;
local plant_seed_offset = 10;

func Construction()
{
	StartGrowth(1);
	inherited(...);
}

private func Initialize()
{
	SetProperty("MeshTransformation", Trans_Rotate(RandomX(0, 359),0, 1, 0));
}

public func Incineration()
{
	CreateParticle("Grass", 0, 0, PV_Random(-20, 20), PV_Random(-20, 10), PV_Random(30, 100), Particles_Straw(), 30);
	RemoveObject();
}

local Name = "$Name$";
local BlastIncinerate = 1;
local ContactIncinerate = 3;
local Placement = 4;