/*
	Squid
	Author: Zapper
	
	The squid will head towards moving objects and try to avoid solid objects.
	To achieve this, force-field attractors and repulsors are placed based on Monte-Carlo sampling of the landscape.
*/

#include Library_ForceField
#include Library_Edible


static const SQUID_SWIM_MAX_SPEED = 30;
static const SQUID_VISION_MAX_RANGE = 100;

local walking, swimming;

local swim_animation, idle_animation, walk_animation, movement_animation_node;
local custom_bone_transform_slot, custom_bone_transform_r;
local base_transform;

// this is a 3D vector with the current direction the squid is facing
local current_orientation, current_speed;
local is_in_idle_animation;
local current_angle; // for smoother turning

local ink_level;

// Whether the squid is friendly or will actively chase Clonks.
local is_friendly;

/*
	Places squid.
	"settings" can contain a boolean property "friendly" which defines whether the squid will be harmless or harmful. Defaults to being harmless.
*/
public func Place(int amount, proplist rectangle, proplist settings)
{
	settings = settings ?? {};
	var friendly = settings.friendly ?? true;
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
		
		if (!friendly)
			f->SetFriendly(false);
		
		// Randomly add some large/slim squid
		if (Random(3))
		{
			// there are naturally smaller and larger squid
			f->SetCon(RandomX(75, 125));
			// make sure the smaller ones don't grow larger any more
			f->StopGrowth(); 
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

public func Construction()
{
	current_orientation = [0, -2, 1];
	current_angle = 0;
	
	SetYZScale(1000);
	
	// general stuff	
	StartGrowth(15);
	
	var len = GetAnimationLength("Float");
	idle_animation = PlayAnimation("Float", 5, Anim_Linear(0, 0, len, 180 + Random(40), ANIM_Loop), Anim_Const(1000));
	var len = GetAnimationLength("Swim");
	swim_animation = PlayAnimation("Swim", 5, Anim_Linear(0, 0, len, 90 + Random(20), ANIM_Loop), Anim_Const(0), idle_animation);
	movement_animation_node = swim_animation + 1;
	is_in_idle_animation = true;
	UpdateSwim();
	
	ink_level = 1000;
	
	SetAction("Swim");
	SetComDir(COMD_None);
	ScheduleCall(this, this.InitActivity,  1 + Random(10), 0);
	AddTimer(this.UpdateSwim, 2);
	
	// The squid is friendly by default.
	SetFriendly(true);
	
	_inherited(...);
	
	// setup of the force fields after the call to inherited(...)
	SetDefaultForceFieldMaxDistance(SQUID_VISION_MAX_RANGE);
	SetDefaultForceFieldTTD(36 * 4);
	SetMaxEmitterNumber(7);
}

public func SetFriendly(bool friendly)
{
	is_friendly = friendly ?? true;
	
	if (is_friendly)
	{
		SetMeshMaterial("SquidMaterialFriendly");
	}
	else
	{
		SetMeshMaterial("SquidMaterial");
	}
}

private func InitActivity()
{
	if (GetAlive()) AddTimer(this.Activity, 10);
}

public func SetYZScale(int new_scale)
{
	base_transform = Trans_Mul(Trans_Rotate(90, 0, 1, 0), Trans_Scale(1000, new_scale, new_scale));
	return true;
}

public func Death()
{
	RemoveTimer(this.UpdateSwim);
	RemoveTimer(this.Activity);
	Decay();
	this.Collectible = true;
	this.MeshTransformation = base_transform;
	
	// fade current animations into death animation
	var len = GetAnimationLength("Die");
	PlayAnimation("Die", 7, Anim_Linear(0, 0, len, 36, ANIM_Hold), Anim_Linear(0, 0, 1000, 10, ANIM_Hold));
		
	return _inherited(...);
}

public func CatchBlow()
{
	if (ink_level >= 1000)
		DoInk();
	else if (ink_level > 500 && !Random(5))
		DoInk();
}

public func NutritionalValue() { if (!GetAlive()) return 15; else return 0; }

private func Activity()
{
	if (swimming)
	{
		UpdateVision();
		DoActions();
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

private func UpdateVision()
{
	var playful_distance = SQUID_VISION_MAX_RANGE;
	if (!is_friendly)
		playful_distance /= 2;
	UpdateVisionFor(FindObjects(Find_Distance(playful_distance), Find_Category(C4D_Object | C4D_Living | C4D_Vehicle), Find_OCF(OCF_HitSpeed2), Find_Exclude(this), Find_NoContainer(), Sort_Distance()));
	
	if (!is_friendly)
	{
		UpdateVisionFor(FindObjects(Find_Distance(SQUID_VISION_MAX_RANGE), Find_OCF(OCF_Alive), Find_Func("IsClonk"), Find_NoContainer(), Sort_Distance()), true);
	}
	else
	{
		// Suddenly find a random object surprisingly interesting.
		if (!Random(10))
			UpdateVisionFor(FindObjects(Find_Distance(playful_distance), Find_Category(C4D_Object | C4D_Living | C4D_Vehicle), Find_Exclude(this), Find_NoContainer(), Sort_Reverse(Sort_Distance()))); 
	}
	UpdateWallVision();
}

private func UpdateVisionFor(array objects, bool is_food)
{
	// get first object from array that is in a certain angle
	for (var obj in objects)
	{
		if (!PathFree(GetX(), GetY(), obj->GetX(), obj->GetY())) continue;
		if ((obj->GetMaterial() != GetMaterial())) continue;
		
		AddAttractor(obj, nil, 250);
		
		if (is_food && ink_level > 800 && ObjectDistance(this, obj) < 20)
		{
			DoInk();
		}
		return true;
	}
	return false;
}

private func UpdateWallVision()
{
	var vision_range = SQUID_VISION_MAX_RANGE / 2;
	// just do a random ray cast and see whether we hit landscape
	var angle = Angle(0, 0, GetXDir(), GetYDir()) + RandomX(-120, +120);
	var distance = 3;
	while (distance < vision_range)
	{
		var x = Sin(angle, distance);
		var y = -Cos(angle, distance);
		if (GetMaterial(x, y) != Material("Water"))
		{
			AddRepulsor(GetX() + x, GetY() + y, 1000, vision_range / 2);
			return;
		}
		distance += 5;
	}
}

private func DoActions()
{
	var result = CalculateForce();
	current_orientation = [result[0], result[1]];
	var speed = Sqrt(result[0]**2 + result[1]**2);
	var max_speed = BoundBy((1000 - ink_level) * SQUID_SWIM_MAX_SPEED / 1000, 10, 2 * SQUID_SWIM_MAX_SPEED);
	current_speed = BoundBy(speed / 50, 2, max_speed);
}

private func UpdateSwim()
{
	ink_level = Min(ink_level + 5, 1000);
	
	if (!swimming) return;
		
	// the movement angle is an interpolation between the current real movement direction and a default "upwards" direction
	
	var velocity = Sqrt(GetXDir() ** 2 + GetYDir() ** 2);
	if (velocity == 0) velocity = 1;
	
	var current = current_orientation;
	var angle = Angle(0, 0, current[0], current[1]);
	if (!current[0] && !current[1]) angle = 180; // drift down
	
	var target_x = BoundBy(Sin(angle, current_speed), GetXDir() - 2, GetXDir() + 2);
	var target_y = BoundBy(-Cos(angle, current_speed), GetYDir() - 2, GetYDir() + 2);
	
	SetXDir(target_x);
	SetYDir(target_y);
	
	// the animation to play depends on the speed of the squid
	var is_fast = velocity >= SQUID_SWIM_MAX_SPEED/3;
	if (is_fast && !is_in_idle_animation) {} // no change needed
	else if (!is_fast && is_in_idle_animation) {} // ok, too
	else
	{
		var current_weight = GetAnimationWeight(movement_animation_node);
		var start = 1000;
		var end = 0;
		
		if (is_in_idle_animation)
		{
			start = 0;
			end = 1000;
		}
		
		SetAnimationWeight(movement_animation_node, Anim_Linear(current_weight, start, end, 30, ANIM_Hold));
		is_in_idle_animation = !is_in_idle_animation;
	}
	
	// Where do we want to go?
	var target_angle = Angle(0, 0, GetXDir(1), GetYDir(1));
	var turn_direction = BoundBy(GetTurnDirection(current_angle, target_angle), -4, +4);
	
	// Move head a bit to simulate water resistance.
	custom_bone_transform_r = BoundBy(custom_bone_transform_r + turn_direction, -45, 45);
	// The head wants to stand upright, though.
	if (custom_bone_transform_r > 0) custom_bone_transform_r -= 1;
	else custom_bone_transform_r += 1;
	// Need to remove the old bone transformation?
	if (custom_bone_transform_slot != nil)
		StopAnimation(custom_bone_transform_slot);
		
	if (custom_bone_transform_r != 0)
		custom_bone_transform_slot = TransformBone("shell", Trans_Rotate(custom_bone_transform_r, 0, 1, 0), 6, Anim_Const(800)); 
	else
		custom_bone_transform_slot = nil;

	
	// And finally, slowly turn to target.
	current_angle = current_angle + turn_direction;
	this.MeshTransformation = Trans_Mul(Trans_Rotate(current_angle, 0, 0, 1), base_transform);
}


private func DoJump()
{
	SetAction("Jump");
	Sound("Hits::SoftTouch*");
	
	var x_dir = RandomX(ActMap.Jump.Speed/2, ActMap.Jump.Speed);
	if (GetComDir() == COMD_Left) x_dir *= -1;
	var y_dir = -RandomX(ActMap.Jump.Speed/3, ActMap.Jump.Speed);
	SetSpeed(x_dir, y_dir);
}

private func StartWalk() 
{
	if (GBackLiquid())
	{
		SetAction("Swim");
		return;
	}
	walking = true;
	swimming = false;
	
	if (walk_animation == nil)
	{
		this.MeshTransformation = base_transform;
		
		var len = GetAnimationLength("Walk");
		walk_animation = PlayAnimation("Walk", 6, Anim_Linear(0, 0, len, 36, ANIM_Loop), Anim_Linear(0, 0, 1000, 10, ANIM_Hold));
	}
}

private func StartSwim()
{
	swimming = true;
	walking = false;
	
	if (walk_animation != nil)
	{
		StopAnimation(walk_animation);
		walk_animation = nil;
	}
	
	// make sure we go down when entering the water
	current_orientation = [RandomX(-10, 10), RandomX(2, 10), 1];
	
	UpdateSwim();
}

private func StartJump()
{
	if (GBackLiquid())
	{
		SetAction("Swim");
		return;
	}
	
	swimming = false;
	walking = false;
}

private func DoInk()
{
	ink_level = 0;
	
	var particles =
	{
		CollisionVertex = 500,
		OnCollision = PC_Stop(),
		ForceY = PV_Random(-2, 2),
		ForceX = PV_Random(-2, 2),
		DampingX = 900, DampingY = 900,
		Alpha = PV_Linear(255, 0),
		R = 50, G = 50, B = 100,
		Size = PV_Linear(PV_Random(10, 20), PV_Random(40, 60)),
		Phase = PV_Random(0, 15)
	};
	CreateParticle("SmokeThick", 0, 0, PV_Random(-40, 40), PV_Random(-40, 40), PV_Random(36, 100), particles, 64);
	// Make squid invisible for some time. Also inverse behavior during invisibility.
	AddEffect("Invisibility", this, 1, 2, this, nil, particles);
	// Drain Clonks' breath a bit faster when in ink.
	AddEffect("IntInkBlob", nil, 1, 5, nil, GetID(), GetX(), GetY());
}

private func FxIntInkBlobStart(object target, effect fx, temp, int x, int y)
{
	if (temp) return;
	fx.x = x;
	fx.y = y;
}

private func FxIntInkBlobTimer(object target, effect fx, int time)
{
	if (time > 30 + Random(20)) return -1;
	var max_distance = 40;
	for (var clonk in FindObjects(Find_Distance(max_distance, fx.x, fx.y), Find_OCF(OCF_Alive), Find_Func("IsClonk")))
	{
		if (!PathFree(fx.x, fx.y, clonk->GetX(), clonk->GetY())) continue;
		var distance = Distance(fx.x, fx.y, clonk->GetX(), clonk->GetY());
		var breath_penalty = 2 * (max_distance - distance);
		clonk->DoBreath(-breath_penalty);
	}
	return FX_OK;
}

private func FxInvisibilityStart(object target, effect fx, temp, particles)
{
	if (temp) return;
	fx.old_visibility = this.Visibility;
	this.Visibility = VIS_None;
	fx.particles = particles;
	SetInverseForceFields(true);
}

private func FxInvisibilityTimer(object target, effect fx, int time)
{
	if (time > 35 * 5) return FX_Execute_Kill;
	if (time > 35 * 2) return FX_OK;
	var size = 2 * 35 - time;
	fx.particles.Size = size/2;
	CreateParticle("SmokeThick", 0, 0, PV_Random(-5, 5), PV_Random(-5, 5), PV_Random(20, 36), fx.particles, 4);
	return FX_OK;
}

private func FxInvisibilityStop(object target, effect fx, int reason, int temp)
{
	if (temp) return;
	fx.particles.Size = PV_Linear(PV_Random(15, 20), PV_Random(0, 5));
	CreateParticle("SmokeThick", 0, 0, PV_Random(-30, 30), PV_Random(-30, 30), PV_Random(20, 36), fx.particles, 32);
	this.Visibility = fx.old_visibility;
	SetInverseForceFields(false);
}

// Only bleeding Squid will be eaten by other predators.
public func IsPrey() { return GetEnergy() < this.MaxEnergy / 3000; }

public func SaveScenarioObject(props)
{
	if (!is_friendly) props->AddCall("Hostile", this, "SetFriendly", false);
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
local MaxEnergy = 80000;
local MaxBreath = 360; // 360 =ten seconds
local Placement = 1;
local NoBurnDecay = true;
local BreatheWater = 1;
local BorderBound = C4D_Border_Sides | C4D_Border_Top | C4D_Border_Bottom;
local ContactCalls = true;

public func Definition(proplist def)
{
	def.PictureTransformation = Trans_Mul(Trans_Translate(0, -1600, 0), Trans_Rotate(20, 1, 0, 0), Trans_Rotate(70, 0, 1, 0));
}

