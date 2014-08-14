/*
	Fish
	Author: Zapper
*/

#include Library_FuzzyLogic

static const FISH_SWIM_MAX_ANGLE = 15;
static const FISH_SWIM_MAX_SPEED = 30;
static const FISH_VISION_MAX_ANGLE = 140;
static const FISH_VISION_MAX_RANGE = 200;

local walking, swimming;
local current_angle, current_speed, current_direction;
local swim_animation;
local base_transform;


func Place(int amount, proplist rectangle, proplist settings)
{
	var max_tries = 2 * amount;
	var loc_area = nil;
	if (rectangle) loc_area = Loc_InRect(rectangle);
	var f;
	
	while ((amount > 0) && (--max_tries > 0))
	{
		var spot = FindLocation(Loc_Material("Water"), Loc_Space(50, false), loc_area);
		if (!spot) continue;
		
		f = CreateObject(this, spot.x, spot.y, NO_OWNER);
		if (!f) continue;
		// Randomly add some large/slim fish
		if (Random(3))
		{
			// there are naturally smaller and larger fishes
			f->SetCon(RandomX(75, 125));
			// make sure the smaller ones don't grow larger any more
			f->StopGrowth(); 
			// slim fish. Large fish must be made slim because otherwise the graphics are clipped D:
			if (f->GetCon() > 100)
				f->SetYZScale(400+Random(300)); 
			else if (!Random(3))
				f->SetYZScale(400+Random(600)); 
		}
		if (f->Stuck())
		{
			f->RemoveObject();
			continue;
		}
		--amount;
	}
	return f; // return last created fish
}

func Construction()
{
	// general stuff	
	StartGrowth(15);
	current_angle = Random(360);
	current_speed = RandomX(FISH_SWIM_MAX_SPEED/5, FISH_SWIM_MAX_SPEED);
	
	var len = GetAnimationLength("Swim");
	swim_animation = PlayAnimation("Swim", 5, Anim_Linear(0, 0, len, 100, ANIM_Loop), Anim_Const(500));
	UpdateSwim();
	
	SetAction("Swim");
	SetComDir(COMD_None);
	ScheduleCall(this, this.InitActivity,  1 + Random(10), 0);
	AddTimer(this.UpdateSwim, 2);
	
	// FUZZY LOGIC INIT BELOW
	_inherited(...);
	InitFuzzyRules();
}

func InitActivity()
{
	if (GetAlive()) AddTimer(this.Activity, 15);
}

func SetYZScale(int new_scale)
{
	base_transform = Trans_Scale(1000, new_scale, new_scale);
	return true;
}

func Death()
{
	RemoveTimer(this.UpdateSwim);
	RemoveTimer(this.Activity);
	this.MeshTransformation = Trans_Rotate(160 + Random(41), 1, 0, 0);
	if (base_transform) this.MeshTransformation = Trans_Mul(base_transform, this.MeshTransformation);
	StopAnimation(swim_animation);
	AddTimer(this.Decaying, 500);
	this.Collectible = true;
	
	// maybe respawn a new fish if roe is near
	var roe = FindObject(Find_Distance(200), Find_ID(FishRoe));
	if (roe)
		roe->Hatch(GetID());
	
	return _inherited(...);
}

func Decaying()
{
	if (GetCon()<20) RemoveObject(); else DoCon(-5);
	return true;
}

protected func ControlUse(object clonk, int iX, int iY)
{
	clonk->Eat(this);
}

public func NutritionalValue() { if (!GetAlive()) return 15; else return 0; }

func InitFuzzyRules()
{
	// ACTION SETS
	AddFuzzySet("swim", "left", [[-FISH_SWIM_MAX_ANGLE, 1], [-FISH_SWIM_MAX_ANGLE/2, 0], [FISH_SWIM_MAX_ANGLE, 0]]);
	AddFuzzySet("swim", "straight", [[-5, 0], [0, 1], [5, 0]]);
	AddFuzzySet("swim", "right", [[-FISH_SWIM_MAX_ANGLE, 0], [FISH_SWIM_MAX_ANGLE/2, 0], [FISH_SWIM_MAX_ANGLE, 1]]);
	
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
	AddFuzzyRule(FuzzyAnd(FuzzyOr("wall=straight", "wall=right"), "wall_range=close"), "swim=left");
	AddFuzzyRule(FuzzyAnd("wall=left", "wall_range=close"), "swim=right");
	
}


func Activity()
{
	if (swimming)
	{
		UpdateVision();
		var actions = FuzzyExec();
		DoActions(actions);
	}
	else if (walking)
	{
		// PANIC WHERE THE WATER AT
		var rx = RandomX(10, 25);
		if (!Random(2)) rx *= -1;
		
		var has_water = false;
		for (var y = 0; y <= 12; y += 4)
		{
			if (!GBackLiquid(rx, y)) continue;
			has_water = true;
			break;
		}
		
		if (has_water)
		{
			if (rx < 0) SetComDir(COMD_Left);
			else SetComDir(COMD_Right);
			
			if (!Random(2))
				DoJump();
			return true;
		}
		else
		{
			if (!Random(2))
			{
				if (!Random(2)) SetComDir(COMD_Left);
				else SetComDir(COMD_Right);
			}
			if (!Random(3))
				DoJump();
		}
	}
	return 1;
}

func UpdateVision()
{
	UpdateVisionFor("enemy", "enemy_range", FindObjects(Find_Distance(FISH_VISION_MAX_RANGE), Find_OCF(OCF_Alive), Find_Or(Find_Func("IsPredator"), Find_Func("IsClonk")), Find_NoContainer(), Sort_Distance()));
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
		
		// prevent piranhas to jump out of the water to eat unsuspecting Clonks
		if (is_food && (obj->GetMaterial() != GetMaterial())) continue;
		
		//CreateParticle("MagicSpark", obj->GetX() - GetX(), obj->GetY() - GetY(), 0, 0, 60, RGB(0, 255, 0));
		//this->Message("%s@%d (me %d, it %d)", obj->GetName(), d, current_angle, angle);
		var distance = ObjectDistance(this, obj);
		Fuzzify(set, d);
		Fuzzify(range_set, distance);
		
		// now that we fuzzified our food - can we actually eat it, too???
		if (is_food && distance < GetCon()/10)
			DoEat(obj);

		return true;
	}
	
	Fuzzify(set, 0);
	Fuzzify(range_set, FISH_VISION_MAX_RANGE + 1);
	return false;
}

func UpdateWallVision()
{
	// anything in this function, that appears weird, looks that way due to optimization..
	
	// asses direction of wall
	var closest = 0;
	var closest_distance = FISH_VISION_MAX_RANGE;
	//for (var angle = -FISH_VISION_MAX_ANGLE/3; angle <= FISH_VISION_MAX_ANGLE; angle += FISH_VISION_MAX_ANGLE/3)
	
	var angle = -FISH_VISION_MAX_ANGLE/5;
	do
	{
		// quickly check solid point
		var max = FISH_VISION_MAX_RANGE/3;
		var px, py;
		for (var d = 5; d <= max; d += 20)
		{
			//var x = (x_fac * d) / 1000;
			//var y = (y_fac * d) / 1000;
			var x = Sin(current_angle + angle, d);
			var y = -Cos(current_angle + angle, d);
			if (GBackLiquid(x, y)) continue;

			px = x;
			py = y;
			break;
		}
		
		/*var px = Sin(current_angle + angle, FISH_VISION_MAX_RANGE);
		var py = -Cos(current_angle + angle, FISH_VISION_MAX_RANGE);		
		var point = PathFree2(GetX(), GetY(), GetX() + px, GetY() + py);
		*/
		if (!px && !py) { angle *= -1; continue; }
		//CreateParticle("MagicSpark", px , py, 0, 0, 60, RGB(255, 0, 0));
		var distance = Distance(0, 0, px, py);
		if (distance >= closest_distance) { angle *= -1; continue; }
		closest = angle;
		closest_distance = distance;
		
		angle *= -1;
	}
	while (angle > 0);
	
	if (closest_distance == FISH_VISION_MAX_RANGE)
	{
		// check for material in front, happens occasionally
		var d = 5;
		if (!GBackLiquid(Sin(current_angle, d), -Cos(current_angle, d)))
		{
			closest_distance = d;
		}
	}
	
	Fuzzify("wall", closest);
	Fuzzify("wall_range", closest_distance);
}

func DoActions(proplist actions)
{
	current_speed = actions.speed;
	current_direction = actions.swim;
}

func UpdateSwim()
{
	if (!swimming) return;
	
	current_angle = (current_angle + current_direction) % 360;
	if (current_angle < 0) current_angle = 360 + current_angle;
	
	SetVelocity(current_angle, current_speed);
	
	var len = GetAnimationLength("Swim");
	var pos = GetAnimationPosition(swim_animation);
	SetAnimationPosition(swim_animation, Anim_Linear(pos, 0, len, FISH_SWIM_MAX_SPEED - current_speed + 1, ANIM_Loop));
	
	var t = current_angle - 270;
	var t2 = Cos(t, 90) - 90; // Expand the areas around 0deg and 180deg a bit so you see fish from the side more
	this.MeshTransformation = Trans_Mul(Trans_Rotate(t, 0, 0, 1), Trans_Rotate(t2, 1, 0, 0), base_transform);
}

func DoEat(object obj)
{
	// fishes can nom on food not only once - they will eat it piece for piece
	var x = obj->GetX() - GetX();
	var y = obj->GetY() - GetY();
	Bubble(1, 0, 0);
	Bubble(1, x, y);
	//CreateParticle("MaterialParticle", x, y, RandomX(-5, 5), RandomX(-5, 5), 20, RGB(50, 25, 0));
	
	var effect = GetEffect("IsBeingEaten", obj);
	if (!effect)
		effect = AddEffect("IsBeingEaten", obj, 1, 0, nil, Fish);
	EffectCall(obj, effect, "Add");
	
	DoEnergy(2);
	
	// happy fishes can lay happy fish eggs
	if (Random(2) && !GetEffect("PlacedRoe"))
		AddEffect("PlaceRoe", this, 1, 30, this);
}

func FxPlaceRoeStart(target, effect, temp)
{
	if (temp) return;
	effect.placed = false;
}

func FxPlaceRoeTimer(target, effect, timer)
{
	if (!effect.placed)
	{
		if (Random(4)) return FX_OK;
		CreateObject(FishRoe, 0, 0, GetOwner());
		effect.placed = true;
		return FX_OK;
	}
	// cooldown for laying eggs!
	if (timer < 35 * 60) return FX_OK;
	return FX_Execute_Kill;
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


func DoJump()
{
	SetAction("Jump");
	Sound("SoftTouch*");
	
	var x_dir = RandomX(ActMap.Jump.Speed/2, ActMap.Jump.Speed);
	if (GetComDir() == COMD_Left) x_dir *= -1;
	var y_dir = -RandomX(ActMap.Jump.Speed/3, ActMap.Jump.Speed);
	SetSpeed(x_dir, y_dir);
}

func StartWalk() 
{
	if (GBackLiquid())
	{
		SetAction("Swim");
		return;
	}
	walking = true;
	swimming = false;
	
	var len = GetAnimationLength("Swim");
	var pos = GetAnimationPosition(swim_animation);
	SetAnimationPosition(swim_animation, Anim_Linear(pos, 0, len, 10, ANIM_Loop));
	this.MeshTransformation = Trans_Mul(Trans_Rotate(90 + RandomX(-10, 10), 1, 0, 0), base_transform);
}

func StartSwim()
{
	swimming = true;
	walking = false;
	
	// make sure we go down when entering the water
	current_direction = RandomX(170, 190);
	
	UpdateSwim();
}
func StartJump()
{
	if (GBackLiquid())
	{
		SetAction("Swim");
		return;
	}
	
	swimming = false;
	walking = false;
}

func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	// Avoid saving some stuff that's reinitialized anyway
	props->Remove("Con"); props->Remove("XDir"); props->Remove("YDir"); props->Remove("ComDir");
	return true;
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
	Delay = 0,
	FacetBase=1,
	NextAction = "Swim",
	StartCall = "StartSwim"
},
Walk = {
	Prototype = Action,
	Name = "Walk",
	Procedure = DFA_WALK,
	Speed = 30,
	Accel = 16,
	Decel = 16,
	Length = 1,
	Delay = 0,
	FacetBase=1,
	Directions = 2,
	FlipDir = 1,
	NextAction = "Walk",
	StartCall = "StartWalk",
	InLiquidAction = "Swim",
},
Jump = {
	Prototype = Action,
	Name = "Jump",
	Procedure = DFA_FLIGHT,
	Speed = 30,
	Accel = 16,
	Decel = 16,
	Length = 1,
	Delay = 0,
	FacetBase=1,
	Directions = 2,
	FlipDir = 1,
	NextAction = "Jump",
	StartCall = "StartJump",
	InLiquidAction = "Swim",
},
Dead = {
	Prototype = Action,
	Name = "Dead",
	Procedure = DFA_NONE,
	Speed = 10,
	Length = 1,
	Delay = 0,
	FacetBase=1,
	Directions = 2,
	FlipDir = 1,
	NextAction = "Hold",
	NoOtherAction = 1,
	ObjectDisabled = 1,
}
};
local Name = "$Name$";
local Description = "$Description$";
local MaxEnergy = 40000;
local MaxBreath = 180; // 180 = five seconds
local Placement = 1;
local NoBurnDecay = 1;
local BreatheWater = 1;

func IsPrey() { return true; }

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(20,1,0,0),Trans_Rotate(70,0,1,0)), def);
}

