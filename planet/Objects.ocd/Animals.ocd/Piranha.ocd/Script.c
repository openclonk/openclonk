/*
	Fish
	Author: Zapper
*/

#include Fish

#include Library_FuzzyLogic

static const FISH_SWIM_MAX_ANGLE = 30;
static const FISH_SWIM_MAX_SPEED = 50;
static const FISH_VISION_MAX_ANGLE = 140;
static const FISH_VISION_MAX_RANGE = 200;


local hunger;

func Construction()
{
	hunger = 0;
	AddTimer("MoreHunger", 40);
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
	AddFuzzySet("hunger", "high", [[25, 0], [100, 1], [100, 100]]);
	
	// RULES
	AddFuzzyRule(FuzzyOr(FuzzyAnd("wall_range=close", "wall=left"), "food=right"), "swim=right");
	AddFuzzyRule(FuzzyOr(FuzzyAnd("wall_range=close", "wall=right"), "food=left"), "swim=left");
	AddFuzzyRule("hunger=high", "speed=fast");
	AddFuzzyRule("hunger=low", "speed=slow");
	
}


func UpdateVision()
{
	Fuzzify("hunger", hunger);
	UpdateVisionFor("food", "food_range", FindObjects(Find_Distance(FISH_VISION_MAX_RANGE), Find_Func("IsPrey"), Find_NoContainer(), Sort_Distance()), true);
	UpdateWallVision();
}

func UpdateVisionFor(string set, string range_set, array objects, bool is_food)
{
	// get first object from array that is in a certain angle
	for (var obj in objects)
	{
		if (!PathFree(GetX(), GetY(), obj->GetX(), obj->GetY())) continue;
		var angle = Angle(GetX(), GetY(), obj->GetX(), obj->GetY());
		var d = GetTurnDirection(current_angle, angle);
		if (!Inside(d, -FISH_VISION_MAX_ANGLE, FISH_VISION_MAX_ANGLE)) continue;
		//CreateParticle("MagicSpark", obj->GetX() - GetX(), obj->GetY() - GetY(), 0, 0, 60, RGB(0, 255, 0));
		//this->Message("%s@%d (me %d, it %d)", obj->GetName(), d, current_angle, angle);
		var distance = ObjectDistance(this, obj);
		Fuzzify(set, d);
		Fuzzify(range_set, distance);
		
		// now that we fuzzified our food - can we actually eat it, too???
		if (is_food && distance < 10)
		{
			Sound("Munch*");
			var len = GetAnimationLength("Swim");
			PlayAnimation("Bite", 5,  Anim_Linear(0, 0, len, 36, ANIM_Remove), Anim_Const(1000));
			if (obj->GetAlive())
				obj->DoEnergy(-5);
			hunger -= 5;
			if (hunger < 0) hunger = 0;
			CreateParticle("MaterialSpark", 0, 0, RandomX(-5, 5), RandomX(-5, 5), 100, RGB(200, 5, 5));
		}
		return true;
	}
	
	Fuzzify(set, 0);
	Fuzzify(range_set, FISH_VISION_MAX_RANGE + 1);
	return false;
}

func UpdateWallVision()
{
	// asses direction of wall
	var closest = 0;
	var closest_distance = FISH_VISION_MAX_RANGE;
	for (var angle = -FISH_VISION_MAX_ANGLE; angle <= FISH_VISION_MAX_ANGLE; angle += FISH_VISION_MAX_ANGLE/3)
	{
		
		// quickly check solid point
		var point = nil;
		for (var d = 5; d <= FISH_VISION_MAX_RANGE; d += 20)
		{
			var x = Sin(current_angle + angle, d);
			var y = -Cos(current_angle + angle, d);
			if (GBackLiquid(x, y)) continue;
			point = [x, y];
			break;
		}
		
		/*var px = Sin(current_angle + angle, FISH_VISION_MAX_RANGE);
		var py = -Cos(current_angle + angle, FISH_VISION_MAX_RANGE);		
		var line = PathFree2(GetX(), GetY(), GetX() + px, GetY() + py);*/
		
		if (!point) continue;
		//CreateParticle("MagicSpark", point[0], point[1], 0, 0, 60, RGB(255, 0, 0));
		var distance = Distance(0, 0, point[0], point[1]);
		if (distance >= closest_distance) continue;
		closest = angle;
		closest_distance = distance;
	}
	Fuzzify("wall", closest);
	Fuzzify("wall_range", closest_distance);
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

