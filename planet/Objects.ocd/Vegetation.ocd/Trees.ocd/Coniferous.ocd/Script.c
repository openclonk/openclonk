/*-- Coniferous Tree --*/

#include Library_Plant

// Overloaded from the plant library to add the foreground parameter, foreground = true will roughly make every 3rd tree foreground (not the offspring though)
func Place(int amount, proplist rectangle, proplist settings, bool foreground)
{
	// Default behaviour
	var trees = inherited(amount, rectangle, settings);
	if (GetLength(trees) < 1) return trees;

	for (var tree in trees)
		if (!Random(3))
			tree.Plane = 510;
	return trees;
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
	// Use Special Vertex Mode 1 (see documentation) so the removed vertex won't come back when rotating the tree.
	SetVertex(0, VTX_Y, 0, 1);
	// Remove the bottom vertex
	RemoveVertex(0);

	_inherited(...);
}

func Damage()
{
	_inherited();

	if (GetDamage() > MaxDamage() && OnFire())
	{
		var burned = CreateObject(Tree_Coniferous_Burned, 0, 0, GetOwner());
		burned->SetCategory(GetCategory());
		burned.Touchable = this.Touchable;
		burned->SetCon(GetCon());
		burned->SetR(GetR());
		burned->Incinerate(OnFire());
		burned->SetPosition(GetX(), GetY());
		Sound("TreeCrack", false);
		RemoveObject();
		return;
	}
}

local Name = "$Name$";
local Touchable = 0;
local BlastIncinerate = 2;
local ContactIncinerate = 6;
local NoBurnDecay = 1;