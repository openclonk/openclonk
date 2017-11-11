/*
	Fish
	Author: Zapper
*/

#include Fish

// Default additional scaling for the mesh.
local BaseScale = +200;
// Damage per bite.
local BiteStrength = 10;

local hunger;

func Construction()
{
	hunger = 0;
	AddTimer("MoreHunger", 80);
	// Set base transform (can be overwritten by e.g. Place()).
	SetYZScale(1000);
	_inherited(...);
}

func MoreHunger()
{
	++hunger;
	if (hunger > 100) hunger = 100;
}

func InitFuzzyRules()
{
	brain = FuzzyLogic->Init();
	
	// ACTION SETS
	brain->AddSet("swim", "sharp_left", [[-2 * SwimMaxAngle, 1], [-SwimMaxAngle, 0], [SwimMaxAngle, 0]]);
	brain->AddSet("swim", "left", [[-SwimMaxAngle, 1], [-SwimMaxAngle/2, 0], [SwimMaxAngle, 0]]);
	brain->AddSet("swim", "straight", [[-5, 0], [0, 1], [5, 0]]);
	brain->AddSet("swim", "right", [[-SwimMaxAngle, 0], [SwimMaxAngle/2, 0], [SwimMaxAngle, 1]]);
	brain->AddSet("swim", "sharp_right", [[-SwimMaxAngle, 0], [SwimMaxAngle, 0], [2 * SwimMaxAngle, 1]]);
	
	brain->AddSet("speed", "slow", [[0, 1], [SwimMaxSpeed/2, 0], [SwimMaxSpeed, 0]]);
	brain->AddSet("speed", "fast", [[0, 0],  [SwimMaxSpeed/2, 0], [SwimMaxSpeed, 1]]);
	
	// RULE SETS
	var directional_sets = ["food"];
	
	for (var set in directional_sets)
	{
		brain->AddSet(set, "left", [[-VisionMaxAngle, 1], [0, 0], [VisionMaxAngle, 0]]);
		brain->AddSet(set, "straight", [[-5, 0], [0, 1], [5, 0]]);
		brain->AddSet(set, "right", [[-VisionMaxAngle, 0], [0, 0], [VisionMaxAngle, 1]]);
	}
	
	var proximity_sets = ["food_range"];
	var middle = VisionMaxRange / 2;
	
	for (var set in proximity_sets)
	{
		brain->AddSet(set, "far", [[middle, 0], [VisionMaxRange, 1], [VisionMaxRange, 1]]);
		brain->AddSet(set, "medium", [[0, 0], [middle, 1], [VisionMaxRange, 0]]);
		brain->AddSet(set, "close", [[0, 1], [0, 1], [middle, 0]]);
	}
	
	brain->AddSet("left_wall", "close", [[0, 1], [0, 1], [wall_vision_range/2, 0]]);
	brain->AddSet("right_wall", "close", [[0, 1], [0, 1], [wall_vision_range/2, 0]]);
	brain->AddSet("wall_range", "close", [[0, 1], [0, 1], [wall_vision_range, 0]]);
	
	brain->AddSet("hunger", "low", [[0, 1], [0, 1], [75, 0]]);
	brain->AddSet("hunger", "high", [[25, 0], [100, 1], [100, 1]]);
	
	// RULES
	brain->AddRule(brain->And("hunger=high", "food=right"), "swim=right");
	brain->AddRule(brain->And("hunger=high", "food=left"), "swim=left");
	brain->AddRule("hunger=high", "speed=fast");
	brain->AddRule(brain->Or("wall_range=close", "hunger=low"), "speed=slow");
	brain->AddRule(brain->And("left_wall=close", brain->Not("right_wall=close")), "swim=sharp_right");
	brain->AddRule("right_wall=close", "swim=sharp_left");
}


func UpdateVision()
{
	brain->Fuzzify("hunger", hunger);
	UpdateVisionFor("food", "food_range", FindObjects(Find_Distance(VisionMaxRange), Find_OCF(OCF_Alive), Find_Func("IsPrey"), Find_NoContainer(), Sort_Distance()), true);
	UpdateWallVision();
}

func DoEat(object obj)
{
	BiteEffect();
	var len = GetAnimationLength("Bite");
	PlayAnimation("Bite", 5,  Anim_Linear(0, 0, len, 36, ANIM_Remove), Anim_Const(1000)); // temp overrides Swim animation in same slot
	if (obj->GetAlive())
		obj->DoEnergy(-BiteStrength);
	hunger -= 20;
	if (hunger < 0) hunger = 0;
	//CastParticles("MaterialParticle", 10, 10, 0, 0, 10, 20, RGB(200, 5, 5), RGB(200, 5, 5));
	
	DoEnergy(BiteStrength);
}

private func BiteEffect()
{
	Sound("Animals::Fish::Munch*");
}

// Make this piranha a little larger than the mesh.
func SetYZScale(int new_scale)
{
	base_transform = Trans_Scale(1000 + BaseScale, new_scale + BaseScale, new_scale + BaseScale);
	return true;
}

local Name = "$Name$";
local Description = "$Description$";
local MaxEnergy = 50000;
local Placement = 1;
local NoBurnDecay = true;
local BreatheWater = 1;
local BorderBound = C4D_Border_Sides | C4D_Border_Top | C4D_Border_Bottom;
local ContactCalls = true;

func IsPrey() { return false; }
func IsPredator() { return true; }

func Definition(proplist def)
{
	def.PictureTransformation = Trans_Mul(Trans_Translate(2000, 0, 0), Trans_Scale(1500), Trans_Rotate(20, 1, 0, 0), Trans_Rotate(70, 0, 1, 0));
}

