/*-- Small Coniferous Tree --*/

#include Library_Plant
#include Library_Tree

private func SeedChance() { return 500; }
private func SeedArea() { return 400; }
private func SeedAmount() { return 12; }

func Construction()
{
	StartGrowth(5);
	inherited(...);
}

private func Initialize()
{
	SetProperty("MeshTransformation", Trans_Mul(Trans_Scale(500,500,500), Trans_Rotate(RandomX(0,359),0,1,0)));
}

public func ChopDown()
{
	// Remove the bottom vertex
	SetVertex(0, VTX_Y, 0, 1);
	RemoveVertex(0);

	_inherited(...);
}

local Name = "$Name$";
local Touchable = 0;
local BlastIncinerate = 1;
local ContactIncinerate = 3;