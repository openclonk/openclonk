/*
	Small Piranha
	Author: Zapper
*/

#include Piranha

local SwimMaxAngle = 10;
local SwimMaxSpeed = 25;
local VisionMaxAngle = 140;
local VisionMaxRange = 100;

// Make this one a bit smaller than the original piranha.
local BaseScale = -200;
local BiteStrength = 5;

public func Construction()
{
	inherited(...);
	SetGraphics(nil, Piranha);
	SetGraphics(nil, Piranha, 1, GFXOV_MODE_Picture);
	SetMeshMaterial("PiranhaSmall");
}

private func InitFuzzyRules()
{
	brain = FuzzyLogic->Init();
	
	// ACTION SETS
	brain->AddSet("swim", "sharp_left", [[-2 * SwimMaxAngle, 1], [-SwimMaxAngle, 0], [SwimMaxAngle, 0]]);
	brain->AddSet("swim", "left", [[-SwimMaxAngle, 1], [-SwimMaxAngle/2, 0], [SwimMaxAngle, 0]]);
	brain->AddSet("swim", "straight", [[-5, 0], [0, 1], [5, 0]]);
	brain->AddSet("swim", "right", [[-SwimMaxAngle, 0], [SwimMaxAngle/2, 0], [SwimMaxAngle, 1]]);
	brain->AddSet("swim", "sharp_right", [[-SwimMaxAngle, 0], [SwimMaxAngle, 0], [2 * SwimMaxAngle, 1]]);
	
	brain->AddSet("speed", "slow", [[0, 1], [2 * SwimMaxSpeed / 3, 0], [SwimMaxSpeed, 0]]);
	brain->AddSet("speed", "fast", [[0, 0],  [SwimMaxSpeed/2, 0], [SwimMaxSpeed, 1]]);
	
	// RULE SETS
	var directional_sets = ["friend", "food"];
	
	for (var set in directional_sets)
	{
		brain->AddSet(set, "left", [[-VisionMaxAngle, 1], [0, 0], [VisionMaxAngle, 0]]);
		brain->AddSet(set, "straight", [[-5, 0], [0, 1], [5, 0]]);
		brain->AddSet(set, "right", [[-VisionMaxAngle, 0], [0, 0], [VisionMaxAngle, 1]]);
	}
	
	// For the food, we allow further vision.
	var far = VisionMaxRange;
	var middle = VisionMaxRange / 2;
	brain->AddSet("food_range", "far", [[middle, 0], [far, 1], [far, 1]]);
	brain->AddSet("food_range", "medium", [[0, 0], [middle, 1], [far, 0]]);
	brain->AddSet("food_range", "close", [[0, 1], [0, 1], [middle, 0]]);
	
	brain->AddSet("left_wall", "close", [[0, 1], [0, 1], [wall_vision_range/2, 0]]);
	brain->AddSet("right_wall", "close", [[0, 1], [0, 1], [wall_vision_range/2, 0]]);
	brain->AddSet("wall_range", "close", [[0, 1], [0, 1], [wall_vision_range, 0]]);
	
	brain->AddSet("hunger", "low", [[0, 1], [0, 1], [75, 0]]);
	brain->AddSet("hunger", "high", [[25, 0], [100, 1], [100, 1]]);
	
	// RULES
	brain->AddRule(brain->Or(brain->And("hunger = high", "food = right"), brain->And("food_range = far", "friend = right")), "swim = right");
	brain->AddRule(brain->Or(brain->And("hunger = high", "food = left"), brain->And("food_range = far", "friend = left")), "swim = left");
	brain->AddRule(brain->Not("food_range = far"), "speed = fast");
	brain->AddRule(brain->Or("wall_range = close", "hunger = low"), "speed = slow");
	brain->AddRule(brain->And("left_wall = close", brain->Not("right_wall = close")), "swim = sharp_right");
	brain->AddRule("right_wall = close", "swim = sharp_left");
}


private func UpdateVision()
{
	brain->Fuzzify("hunger", hunger);
	UpdateVisionFor("food", "food_range", FindObjects(Find_Distance(VisionMaxRange), Find_OCF(OCF_Alive), Find_Func("IsPrey"), Find_NoContainer(), Sort_Distance()), true);
	UpdateVisionFor("friend", nil, FindObjects(Find_Distance(VisionMaxRange), Find_ID(GetID()), Find_Exclude(this), Find_NoContainer(), Sort_Distance()));
	UpdateWallVision();
}

private func BiteEffect()
{
	Sound("Animals::Fish::Munch*", {pitch = 100});
}

local Name = "$Name$";
local Description = "$Description$";
local MaxEnergy = 25000;
