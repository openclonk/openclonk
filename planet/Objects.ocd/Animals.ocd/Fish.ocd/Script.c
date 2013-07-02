/*
	Fish
	Author: Zapper
*/

#include Library_FuzzyLogic

static const FISH_SWIM_MAX_ANGLE = 30;
static const FISH_SWIM_MAX_SPEED = 50;
static const FISH_VISION_MAX_ANGLE = 140;
static const FISH_VISION_MAX_RANGE = 200;

local current_angle, current_speed, current_direction;
local swim_animation;

func Place(int amount, proplist rectangle, proplist settings)
{
	var max_tries = 2 * amount;
	var loc_area = nil;
	if (rectangle) loc_area = Loc_InRect(rectangle);
	
	while ((amount > 0) && (--max_tries > 0))
	{
		var spot = FindLocation(Loc_Material("Water"), Loc_Space(10, false), loc_area);
		if (!spot) continue;
		
		var f = CreateObject(this, spot.x, spot.y, NO_OWNER);
		if (f->Stuck())
		{
			f->RemoveObject();
			continue;
		}
		--amount;
	}
	return true;
}

func Construction()
{
	// general stuff	
	StartGrowth(15);
	current_angle = Random(360);
	current_speed = RandomX(FISH_SWIM_MAX_SPEED/5, FISH_SWIM_MAX_SPEED);
	SetAction("Swim");
	SetComDir(COMD_None);
	var len = GetAnimationLength("Swim");
	swim_animation = PlayAnimation("Swim", 5, Anim_Linear(0, 0, len, 100, ANIM_Loop), Anim_Const(500));
	UpdateSwim();
	
	// FUZZY LOGIC INIT BELOW
	_inherited(...);
	InitFuzzyRules();
}

func InitFuzzyRules()
{
	// ACTION SETS
	AddFuzzySet("swim", "left", [[-FISH_SWIM_MAX_ANGLE, 1], [0, 0], [FISH_SWIM_MAX_ANGLE, 0]]);
	AddFuzzySet("swim", "straight", [[-5, 0], [0, 1], [5, 0]]);
	AddFuzzySet("swim", "right", [[-FISH_SWIM_MAX_ANGLE, 0], [0, 0], [FISH_SWIM_MAX_ANGLE, 1]]);
	
	AddFuzzySet("speed", "slow", [[0, 1], [FISH_SWIM_MAX_SPEED/2, 0], [FISH_SWIM_MAX_SPEED, 0]]);
	AddFuzzySet("speed", "fast", [[0, 0],  [FISH_SWIM_MAX_SPEED/2, 0], [FISH_SWIM_MAX_SPEED, 1]]);
	
	// RULE SETS
	var directional_sets = ["friend", "enemy", "food", "wall"];
	
	for (var set in directional_sets)
	{
		AddFuzzySet(set, "left", [[-FISH_VISION_MAX_ANGLE, 1], [0, 0], [FISH_VISION_MAX_ANGLE, 0]]);
		AddFuzzySet(set, "straight", [[-5, 0], [0, 1], [5, 0]]);
		AddFuzzySet(set, "right", [[-FISH_VISION_MAX_ANGLE, 0], [0, 0], [FISH_VISION_MAX_ANGLE, 1]]);
	}
	
	var proximity_sets = ["friend_range", "enemy_range", "food_range"];
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
	
	// RULES
	AddFuzzyRule("enemy_range=close", "speed=fast");
	AddFuzzyRule(FuzzyOr("friend_range=close", "food_range=close", "wall_range=close"), "speed=slow");
	
	AddFuzzyRule(FuzzyAnd(FuzzyNot("wall_range=close"), FuzzyOr("food=left", FuzzyAnd("friend=left", "enemy_range=far", "food_range=far"), "enemy=right")), "swim=left");
	AddFuzzyRule(FuzzyAnd(FuzzyNot("wall_range=close"), FuzzyOr("food=right", FuzzyAnd("friend=right", "enemy_range=far", "food_range=far"), "enemy=left")), "swim=right");
	AddFuzzyRule(FuzzyAnd("wall=right", "wall_range=close"), "swim=left");
	AddFuzzyRule(FuzzyAnd("wall=left", "wall_range=close"), "swim=right");
	
}


protected func Initialize()
{
	SetAction("Swim");
	SetComDir(COMD_None);
	Schedule(this, "AddTimer(\"Activity\", 15)",  1 + Random(10), 0);
	AddTimer("Activity", 15);
	AddTimer("UpdateSwim", 5);
	return 1;
}

	
func Activity()
{
	UpdateVision();
	var actions = FuzzyExec();
	DoActions(actions);
	return 1;
}

func UpdateVision()
{
	UpdateVisionFor("enemy", "enemy_range", FindObjects(Find_Distance(FISH_VISION_MAX_RANGE), Find_Or(Find_Func("IsPredator"), Find_Func("IsClonk")), Find_NoContainer(), Sort_Distance()));
	UpdateVisionFor("friend", "friend_range", FindObjects(Find_Distance(FISH_VISION_MAX_RANGE), Find_ID(GetID()), Find_Exclude(this), Find_NoContainer(), Sort_Distance()));
	UpdateVisionFor("food", "food_range", FindObjects(Find_Distance(FISH_VISION_MAX_RANGE), Find_Func("NutritionalValue"), Find_NoContainer(), Sort_Distance()), true);
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
			// fishes can nom on food not only once - they will eat it piece for piece
			var x = obj->GetX() - GetX();
			var y = obj->GetY() - GetY();
			Bubble(1, 0, 0);
			Bubble(1, x, y);
			CreateParticle("MaterialSpark", x, y, RandomX(-5, 5), RandomX(-5, 5), 50, RGB(50, 25, 0));
			
			var effect = GetEffect("IsBeingEaten", obj);
			if (!effect)
				effect = AddEffect("IsBeingEaten", obj, 1, 0, nil, Fish);
			EffectCall(obj, effect, "Add");
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

func DoActions(proplist actions)
{
	current_speed = actions.speed;
	current_direction = actions.swim;
	UpdateSwim();
}

func UpdateSwim()
{
	current_angle = (current_angle + current_direction) % 360;
	if (current_angle < 0) current_angle = 360 + current_angle;
	
	SetVelocity(current_angle, current_speed);
	
	var len = GetAnimationLength("Swim");
	var pos = GetAnimationPosition(swim_animation);
	SetAnimationPosition(swim_animation, Anim_Linear(pos, 0, len, 2 * FISH_SWIM_MAX_SPEED, ANIM_Loop));
	
	var t = current_angle - 270;
	this.MeshTransformation = Trans_Mul(Trans_Rotate(t, 0, 0, 1), Trans_Rotate(t, 1, 0, 0));
}

func FxIsBeingEatenStart(target, effect, temp)
{
	if (temp) return;
	effect.amount = 0;
}

func FxIsBeingEatenAdd(target, effect)
{
	if (!target) return;
	if (++effect.amount < target->~NutritionalValue()) return;
	target->RemoveObject();
}

/* Contact */
protected func ContactBottom()
{
	return 1;
}



local ActMap = {

Swim = {
	Prototype = Action,
	Name = "Swim",
	Procedure = DFA_SWIM,
	Speed = 100,
	Accel = 16,
	Decel = 16,
	Length = 1,
	Delay = 10,
	X = 0,
	Y = 0,
	Wdt = 24,
	Hgt = 24,
	NextAction = "Swim",
}
};
local Name = "$Name$";
local Description = "$Description$";
local MaxEnergy = 40000;
local Placement = 1;
local NoBurnDecay = 1;
local BreatheWater = 1;

func IsPrey() { return true; }

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(20,1,0,0),Trans_Rotate(70,0,1,0)), def);
}

