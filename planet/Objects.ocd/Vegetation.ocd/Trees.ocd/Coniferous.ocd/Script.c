/*-- Coniferous Tree --*/

#include Library_Plant

// Overloaded from the plant library to add the foreground parameter, foreground = true will roughly make every 3rd tree foreground (not the offspring though)
func Place(int amount, proplist rectangle, proplist settings, bool foreground)
{
	// Default behaviour
	var trees = inherited(amount, rectangle, settings);
	if (GetLength(trees) < 1) return;

	for (var tree in trees)
		if (!Random(3))
			tree.Plane = 510;
}

private func SeedChance() {	return 500; }
private func SeedArea() { return 400; }
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
	// Remove the bottom vertex
	SetVertex(0, VTX_Y, 0, 1);
	RemoveVertex(0);

	_inherited(...);
}

local Name = "$Name$";
local Touchable = 0;
local BlastIncinerate = 1;
local ContactIncinerate = 3;