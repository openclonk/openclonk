/*-- Yet Another Additional Coniferous Tree --*/

#include Library_Plant
#include Library_Tree

private func SeedChance() { return 500; }
private func SeedArea() { return 400; }
private func SeedAmount() { return 10; }

local lib_tree_burned = Tree_Coniferous4_Burned;

public func GetTreetopPosition(pos)
{
	return Shape->Rectangle(-10,-8, 20,10)->GetRandomPoint(pos);
}

private func Definition(def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(-25000, -8000, 22000), Trans_Rotate(40,0,0,1), Trans_Rotate(-10,1), Trans_Rotate(50,0,1)), def);
}

local Name = "$Name$";
local Touchable = 0;
local BlastIncinerate = 2;
local ContactIncinerate = 6;
local NoBurnDecay = 1;
local Components = {Wood = 3};