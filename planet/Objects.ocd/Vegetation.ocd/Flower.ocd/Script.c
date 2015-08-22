/**
	Flower
	The beauty in nature

	@author Nachtfalter, Armin
*/

#include Library_Plant

private func SeedChance() { return 300; }
private func SeedArea() { return 120; }
private func SeedAmount() { return 6; }

func Construction()
{
	StartGrowth(1);
	var skin = 1 + Random(3);
	if (skin != 1)
		SetMeshMaterial(Format("flower%d", skin));
	SetProperty("MeshTransformation", Trans_Mul(Trans_Scale(RandomX(850,1200)), Trans_Rotate(RandomX(0,359),0,1,0)));
	
	inherited(...);
}

public func Incineration()
{
	CreateParticle("Grass", 0, 0, PV_Random(-20, 20), PV_Random(-20, 10), PV_Random(30, 100), Particles_Straw(), 10);
	RemoveObject();
}

local Name = "$Name$";
local BlastIncinerate = 1;
local ContactIncinerate = 3;
local Placement = 4;
