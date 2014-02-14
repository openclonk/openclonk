/*-- Fern --*/

#include Library_Plant

private func SeedChance() { return 400; }
private func SeedArea() { return 120; }
private func SeedAmount() { return 4; }

func Construction()
{
	StartGrowth(1);
	inherited(...);
}

private func Initialize()
{
	SetProperty("MeshTransformation", Trans_Rotate(RandomX(0,359),0,1,0));
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