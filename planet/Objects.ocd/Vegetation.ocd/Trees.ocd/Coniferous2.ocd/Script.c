/*-- Another Coniferous Tree --*/

#include Library_Plant
#include Library_Tree

private func SeedChance() { return 500; }
private func SeedArea() { return 400; }
private func SeedAmount() { return 10; }

local lib_tree_burned = Tree_Coniferous2_Burned;

public func GetTreetopPosition(pos)
{
	return Shape->Rectangle(-25,-15, 20,30)->GetRandomPoint(pos);
}

private func Definition(def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(-25000, -8000, 35000), Trans_Rotate(35,0,0,1), Trans_Rotate(-15,1)), def);
}

local Name = "$Name$";
local Touchable = 0;
local BlastIncinerate = 2;
local ContactIncinerate = 6;
local NoBurnDecay = 1;