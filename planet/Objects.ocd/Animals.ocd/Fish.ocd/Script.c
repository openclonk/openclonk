/*
	Fish
	Author: Zapper
*/

#include Library_Edible

local SwimMaxAngle = 15;
local SwimMaxSpeed = 30;
local VisionMaxAngle = 140;
local VisionMaxRange = 200;

local walking, swimming;
local current_angle, current_speed, current_direction;
local swim_animation;
local base_transform;

local brain;

// Wall vision behaves slightly differently and it does not make much sense for other fishes to overload these values.
local wall_vision_range = 64;
local wall_vision_max_angle = 35;

func Place(int amount, proplist rectangle, proplist settings)
{
	var max_tries = 2 * amount;
	var loc_area = nil;
	if (rectangle) loc_area = Loc_InArea(rectangle);
	var f;
	
	while ((amount > 0) && (--max_tries > 0))
	{
		var spot = FindLocation(Loc_Material("Water"), Loc_Space(20), loc_area);
		if (!spot) continue;
		
		f = CreateObjectAbove(this, spot.x, spot.y, NO_OWNER);
		if (!f) continue;
		// Randomly add some large/slim fish
		if (Random(3))
		{
			// There are naturally smaller and larger fishes. Fishes above around 150 can be clipped, so stay below that.
			f->SetCon(RandomX(75, 140));
			// make sure the smaller ones don't grow larger any more
			f->StopGrowth(); 
			// Slim fish.
			if (f->GetCon() > 100 || !Random(3))
				f->SetYZScale(1000 - Random(300));  
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

public func IsAnimal() { return true; }

func Construction()
{
	// general stuff	
	StartGrowth(15);
	current_angle = Random(360);
	current_speed = RandomX(SwimMaxSpeed/5, SwimMaxSpeed);
	
	var len = GetAnimationLength("Swim");
	swim_animation = PlayAnimation("Swim", 5, Anim_Linear(0, 0, len, 100, ANIM_Loop), Anim_Const(500));
	UpdateSwim();
	
	SetAction("Swim");
	SetComDir(COMD_None);
	ScheduleCall(this, this.InitActivity,  1 + Random(10), 0);
	AddTimer(this.UpdateSwim, 2);
	InitFuzzyRules();
	
	return _inherited(...);
}

func InitActivity()
{
	if (GetAlive()) AddTimer(this.Activity, 10 + Random(5));
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
	Decay();
	this.Collectible = true;
	
	// maybe respawn a new fish if roe is near
	var roe = FindObject(Find_Distance(200), Find_ID(FishRoe));
	if (roe)
		roe->Hatch(GetID());
	
	return _inherited(...);
}

public func NutritionalValue() { if (!GetAlive()) return 15; else return 0; }

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
	var directional_sets = ["friend", "enemy", "food"];
	
	for (var set in directional_sets)
	{
		brain->AddSet(set, "left", [[-VisionMaxAngle, 1], [0, 0], [VisionMaxAngle, 0]]);
		brain->AddSet(set, "straight", [[-5, 0], [0, 1], [5, 0]]);
		brain->AddSet(set, "right", [[-VisionMaxAngle, 0], [0, 0], [VisionMaxAngle, 1]]);
	}

	var proximity_sets = ["friend_range", "enemy_range", "food_range"];
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
	
	// RULES
	brain->AddRule(brain->And(brain->Not("wall_range = close"), "enemy_range = close"), "speed = fast");
	brain->AddRule(brain->Or("friend_range = close", "food_range = close", "wall_range = close"), "speed = slow");
	
	brain->AddRule(brain->And(brain->Not("wall_range = close"), brain->Or("food = left", brain->And("friend = left", "enemy_range = far", "food_range = far"), "enemy = right")), "swim = left");
	brain->AddRule(brain->And(brain->Not("wall_range = close"), brain->Or("food = right", brain->And("friend = right", "enemy_range = far", "food_range = far"), "enemy = left")), "swim = right");
	brain->AddRule(brain->And("left_wall = close", brain->Not("right_wall = close")), "swim = sharp_right");
	brain->AddRule("right_wall = close", "swim = sharp_left");
}


func Activity()
{
	if (swimming)
	{
		UpdateVision();
		var actions = brain->Execute();
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
	UpdateVisionFor("enemy", "enemy_range", FindObjects(Find_Distance(VisionMaxRange), Find_OCF(OCF_Alive), Find_Or(Find_Func("IsPredator"), Find_Func("IsClonk")), Find_NoContainer(), Sort_Distance()));
	UpdateVisionFor("friend", "friend_range", FindObjects(Find_Distance(VisionMaxRange), Find_ID(GetID()), Find_Exclude(this), Find_NoContainer(), Sort_Distance()));
	UpdateVisionFor("food", "food_range", FindObjects(Find_Distance(VisionMaxRange), Find_Func("NutritionalValue"), Find_NoContainer(), Sort_Distance()), true);
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
		if (!Inside(d, -VisionMaxAngle, VisionMaxAngle) || d == 0) continue;
		
		// prevent piranhas to jump out of the water to eat unsuspecting Clonks
		if (is_food && (obj->GetMaterial() != GetMaterial())) continue;
		
		//CreateParticle("MagicSpark", obj->GetX() - GetX(), obj->GetY() - GetY(), 0, 0, 60, RGB(0, 255, 0));
		//this->Message("%s@%d (me %d, it %d)", obj->GetName(), d, current_angle, angle);
		var angle = -VisionMaxAngle;
		if (d > 0) angle = VisionMaxAngle;
		var distance = ObjectDistance(this, obj);
		brain->Fuzzify(set, angle * distance / VisionMaxRange);
		if (range_set != nil)
			brain->Fuzzify(range_set, distance);
		
		// now that we fuzzified our food - can we actually eat it, too???
		if (is_food && distance < GetCon()/10)
			DoEat(obj);

		return true;
	}
	
	brain->Fuzzify(set, 0);
	if (range_set != nil)
		brain->Fuzzify(range_set, VisionMaxRange + 1);
	return false;
}

func UpdateWallVision()
{
	if (!Random(5))
	{
		// check for material in front, happens occasionally
		var d = 10;
		if (!GBackLiquid(Sin(current_angle, d), -Cos(current_angle, d)))
		{
			brain->Fuzzify("wall_range", d);
			brain->Fuzzify("left_wall", d);
			brain->Fuzzify("right_wall", wall_vision_range);
			return;
		}
	}
	
	// anything in this function, that appears weird, looks that way due to optimization..
	var closest_wall = wall_vision_range;
	
	for (var i = 0; i <= 1; ++i)
	{
		var what = "left_wall";
		var angle = -wall_vision_max_angle;
		if (i == 1)
		{
			angle = +wall_vision_max_angle;
			what = "right_wall";
		}
		
		// quickly check solid point
		var distance = wall_vision_range;
		for (var d = 1; d <= wall_vision_range; d *= 2)
		{
			var x = Sin(current_angle + angle, d);
			var y = -Cos(current_angle + angle, d);
			if (GBackLiquid(x, y)) continue;

			distance = Distance(0, 0, x, y);
			if (distance < closest_wall) closest_wall = distance;
			
			// CreateParticle("SphereSpark", x, y, 0, 0, 30, {Prototype = Particles_MagicRing(), Attach = nil}, 1);
			break;
		}
		
		brain->Fuzzify(what, distance);
	}
		
	brain->Fuzzify("wall_range", closest_wall);
}

func DoActions(proplist actions)
{
	current_speed = actions.speed;
	if (current_speed == 0) current_speed = SwimMaxSpeed / 3; 
	current_direction = actions.swim;
	if (current_direction == 0) current_direction = RandomX(-2, 2);
}

func UpdateSwim()
{
	if (!swimming) return;
	
	current_angle = (current_angle + current_direction) % 360;
	if (current_angle < 0) current_angle = 360 + current_angle;
	
	SetVelocity(current_angle, current_speed);
	
	var len = GetAnimationLength("Swim");
	var pos = GetAnimationPosition(swim_animation);
	SetAnimationPosition(swim_animation, Anim_Linear(pos, 0, len, SwimMaxSpeed - current_speed + 1, ANIM_Loop));
	
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
		CreateObjectAbove(FishRoe, 0, 0, GetOwner());
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
	Sound("Hits::SoftTouch*");
	
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
	FacetBase = 1,
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
	FacetBase = 1,
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
	FacetBase = 1,
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
	FacetBase = 1,
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
local NoBurnDecay = true;
local BreatheWater = 1;
local BorderBound = C4D_Border_Sides | C4D_Border_Top | C4D_Border_Bottom;
local ContactCalls = true;

func IsPrey() { return true; }

public func Definition(proplist def)
{
	def.PictureTransformation = Trans_Mul(Trans_Translate(0, 600, 0), Trans_Scale(1200), Trans_Rotate(20, 1, 0, 0), Trans_Rotate(70, 0, 1, 0));
}
