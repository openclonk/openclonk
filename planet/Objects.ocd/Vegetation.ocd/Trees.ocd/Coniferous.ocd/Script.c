/*-- Coniferous Tree --*/

#include Library_Plant

private func SeedAreaSize() { return 400; }
private func SeedChance() {	return 50; }
private func SeedAmount() { return 12; }

func Construction()
{
	StartGrowth(5);
	// set random rotation so trees don't look alike too much
	SetProperty("MeshTransformation", Trans_Rotate(RandomX(0,359),0,1,0));
	inherited(...);
}

public func IsTree() { return true; }

public func ChopDown()
{
	// Remove the bottom vertex (via removing collision)
	// this is done because Removing the Vertex would re-add it at the first rotation/con-update
	SetVertex(0, VTX_CNAT, CNAT_NoCollision, 2);

	_inherited(...);
}

local Name = "$Name$";
local Touchable = 0;