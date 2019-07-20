/*-- Yet Another Coniferous Tree --*/

#include Library_Plant
#include Library_Tree

local plant_seed_chance = 20;
local plant_seed_area = 400;
local plant_seed_amount = 10;

local lib_tree_burned = Tree_Coniferous3_Burned;

public func GetTreetopPosition(pos)
{
	return Shape->Rectangle(-10,-8, 20, 10)->GetRandomPoint(pos);
}

public func Definition(def, ...)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(-25000, -8000, 22000), Trans_Rotate(40, 0, 0, 1), Trans_Rotate(-10, 1)), def);
	return _inherited(def, ...);
}

local Name = "$Name$";
local Touchable = 0;
local BlastIncinerate = 2;
local ContactIncinerate = 6;
local NoBurnDecay = true;
local Components = {Wood = 3};