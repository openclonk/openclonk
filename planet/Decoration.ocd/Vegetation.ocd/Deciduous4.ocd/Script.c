/*-- Yet Another Additional Deciduous Tree --*/

#include Library_Plant
#include Library_Tree

local plant_seed_chance = 20;
local plant_seed_area = 400;
local plant_seed_amount = 10;
local plant_seed_offset = 50;

local lib_tree_burned = Deco_Tree_Deciduous4_Burned;

public func GetTreetopPosition(pos)
{
	return Shape->Rectangle(-30,-30, 60, 80)->GetRandomPoint(pos);
}

public func Construction() {
	var ret = _inherited(...);
	StartSwing();
	return ret;
}

public func ChopDown() {
	StopSwing();
	return _inherited(...);
}

public func StartSwing() {
	var anim_length = GetAnimationLength("swing");
	PlayAnimation("swing", 1, Anim_Linear(Random(anim_length), 0, anim_length, 500, ANIM_Loop));
}

public func StopSwing() {
	var anim_length = GetAnimationLength("swing");
	PlayAnimation("swing", 1, Anim_Const(GetAnimationPosition(GetRootAnimation(1))));
}

local Name = "$Name$";
local Touchable = 0;
local BlastIncinerate = 2;
local ContactIncinerate = 6;
local NoBurnDecay = true;
local Components = {Wood = 4};