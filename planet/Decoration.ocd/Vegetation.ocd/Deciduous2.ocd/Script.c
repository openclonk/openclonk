/*-- Another Deciduous Tree --*/

#include Library_Plant
#include Library_Tree

local plant_seed_chance = 20;
local plant_seed_area = 400;
local plant_seed_amount = 10;

local lib_tree_burned = Deco_Tree_Deciduous2_Burned;

public func GetTreetopPosition(pos)
{
	return Shape->Rectangle(-28,-20, 46,40)->GetRandomPoint(pos);
}

local Name = "$Name$";
local Touchable = 0;
local BlastIncinerate = 2;
local ContactIncinerate = 6;
local NoBurnDecay = true;
local Components = {Wood = 5};