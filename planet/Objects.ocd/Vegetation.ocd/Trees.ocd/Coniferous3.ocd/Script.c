/*-- Yet Another Coniferous Tree --*/

#include Library_Plant
#include Library_Tree

private func SeedChance() { return 500; }
private func SeedArea() { return 400; }
private func SeedAmount() { return 10; }

public func GetTreetopPosition(pos)
{
	return Shape->Rectangle(-10,-8, 20,10)->GetRandomPoint(pos);
}

local lib_tree_burned = Tree_Coniferous3_Burned;

local Name = "$Name$";
local Touchable = 0;
local BlastIncinerate = 2;
local ContactIncinerate = 6;
local NoBurnDecay = 1;