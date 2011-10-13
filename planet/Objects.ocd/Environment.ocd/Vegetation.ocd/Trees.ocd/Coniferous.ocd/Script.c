/*-- Coniferous Tree --*/

#include Library_Plant

private func SeedAreaSize() { return 400; }

func Construction()
{
	StartGrowth(1);
	inherited(...);
}

private func Initialize()
{
	SetProperty("MeshTransformation", Trans_Rotate(RandomX(0,359),0,1,0));
}

public func IsTree() { return true; }
public func IsStanding() { return true; }

local Name = "$Name$";