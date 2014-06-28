/*
	Fish
	Author: Zapper
*/

#include Fish

#include Library_FuzzyLogic


local hunger;

func Construction()
{
	hunger = 0;
	AddTimer("MoreHunger", 80);
	_inherited(...);
}

func MoreHunger()
{
	++hunger;
	if (hunger > 100) hunger = 100;
}

func InitFuzzyRules()
{
	// ACTION SETS
	AddFuzzySet("swim", "left", [[-FISH_SWIM_MAX_ANGLE, 1], [-FISH_SWIM_MAX_ANGLE/2, 0], [FISH_SWIM_MAX_ANGLE, 0]]);
	AddFuzzySet("swim", "straight", [[-5, 0], [0, 1], [5, 0]]);
	AddFuzzySet("swim", "right", [[-FISH_SWIM_MAX_ANGLE, 0], [FISH_SWIM_MAX_ANGLE/2, 0], [FISH_SWIM_MAX_ANGLE, 1]]);
	
	AddFuzzySet("speed", "slow", [[0, 1], [FISH_SWIM_MAX_SPEED/2, 0], [FISH_SWIM_MAX_SPEED, 0]]);
	AddFuzzySet("speed", "fast", [[0, 0],  [FISH_SWIM_MAX_SPEED/2, 0], [FISH_SWIM_MAX_SPEED, 1]]);
	
	// RULE SETS
	var directional_sets = ["food", "wall"];
	
	for (var set in directional_sets)
	{
		AddFuzzySet(set, "left", [[-FISH_VISION_MAX_ANGLE, 1], [0, 0], [FISH_VISION_MAX_ANGLE, 0]]);
		AddFuzzySet(set, "straight", [[-5, 0], [0, 1], [5, 0]]);
		AddFuzzySet(set, "right", [[-FISH_VISION_MAX_ANGLE, 0], [0, 0], [FISH_VISION_MAX_ANGLE, 1]]);
	}
	
	var proximity_sets = ["food_range"];
	var middle = FISH_VISION_MAX_RANGE / 2;
	var quarter = FISH_VISION_MAX_RANGE / 4;
	
	for (var set in proximity_sets)
	{
		AddFuzzySet(set, "far", [[middle, 0], [FISH_VISION_MAX_RANGE, 1], [FISH_VISION_MAX_RANGE, 1]]);
		AddFuzzySet(set, "medium", [[0, 0], [middle, 1], [FISH_VISION_MAX_RANGE, 0]]);
		AddFuzzySet(set, "close", [[0, 1], [0, 1], [middle, 0]]);
	}
	
	AddFuzzySet("wall_range", "far", [[middle, 0], [FISH_VISION_MAX_RANGE, 1], [FISH_VISION_MAX_RANGE, 1]]);
	AddFuzzySet("wall_range", "medium", [[0, 0], [middle, 1], [FISH_VISION_MAX_RANGE, 0]]);
	AddFuzzySet("wall_range", "close", [[0, 1], [0, 1], [quarter, 0]]);
	
	
	AddFuzzySet("hunger", "low", [[0, 1], [0, 1], [75, 0]]);
	AddFuzzySet("hunger", "high", [[25, 0], [100, 1], [100, 1]]);
	
	// RULES
	AddFuzzyRule(FuzzyOr(FuzzyAnd("wall_range=close", "wall=left"), FuzzyAnd("hunger=high", "food=right")), "swim=right");
	AddFuzzyRule(FuzzyOr(FuzzyAnd("wall_range=close", "wall=right"),  FuzzyAnd("hunger=high", "food=left")), "swim=left");
	AddFuzzyRule("hunger=high", "speed=fast");
	AddFuzzyRule(FuzzyOr("wall_range=close", "hunger=low"), "speed=slow");
	
}


func UpdateVision()
{
	Fuzzify("hunger", hunger);
	UpdateVisionFor("food", "food_range", FindObjects(Find_Distance(FISH_VISION_MAX_RANGE), Find_OCF(OCF_Alive), Find_Func("IsPrey"), Find_NoContainer(), Sort_Distance()), true);
	UpdateWallVision();
}

func DoEat(object obj)
{
	Sound("FishMunch*");
	var len = GetAnimationLength("Swim");
	PlayAnimation("Bite", 5,  Anim_Linear(0, 0, len, 36, ANIM_Remove), Anim_Const(1000));
	if (obj->GetAlive())
		obj->DoEnergy(-5);
	hunger -= 10;
	if (hunger < 0) hunger = 0;
	//CastParticles("MaterialParticle", 10, 10, 0, 0, 10, 20, RGB(200, 5, 5), RGB(200, 5, 5));
	
	DoEnergy(5);
}

local Name = "$Name$";
local Description = "$Description$";
local MaxEnergy = 50000;
local Placement = 1;
local NoBurnDecay = 1;
local BreatheWater = 1;

func IsPrey() { return false; }
func IsPredator() { return true; }

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(20,1,0,0),Trans_Rotate(70,0,1,0)), def);
}

