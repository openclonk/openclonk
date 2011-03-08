/*-- Coniferous Tree --*/

#include Library_Plant

private func SeedAreaSize() { return 500; }
private func SeedAmount() { return 10; }

func Construction()
{
	StartGrowth(1);
}

private func Initialize()
{
	SetProperty("MeshTransformation", Trans_Rotate(RandomX(0,359),0,1,0));
}

local Name = "$Name$";
