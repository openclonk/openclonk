/*
	Butterfly
	Author: Randrian, Ringwaul, Clonkonaut

	A small fluttering being.
*/

#include Library_Insect

local fly_anim, fly_anim_len;

public func Construction(...)
{
	StartGrowth(15);
	fly_anim_len = GetAnimationLength("Fly");
	fly_anim = PlayAnimation("Fly", 1, Anim_Linear(0,0, fly_anim_len, 10, ANIM_Loop));
	this.MeshTransformation = Trans_Rotate(270,1,1,1);
	SetAction("Fly");

	// Make butterflies a bit more colorful.
	SetClrModulation(HSL(Random(256), 255, 100 + Random(60)));

	lib_insect_max_dist = 300;
	lib_insect_shy = true;
	return _inherited(...);
}

/* Insect library */

local attraction;

private func GetAttraction(proplist coordinates)
{
	// Sometimes I don't want to fly to a plant
	if (!Random(7)) return false;
	for (var plant in FindObjects(Find_Distance(150), Find_Or(Find_Func("IsPlant"), Find_ID(Grass)), Sort_Distance()))
	{
		if (!Random(4))
			continue;
		if (ObjectDistance(plant) < 20) // Too close
			continue;
		if (plant->GetCon() < 30) // Too small
			continue;
		if (plant->GBackSemiSolid()) // Under water or covered with solid material
			continue;
		var width = plant->GetObjWidth();
		var height = plant->GetObjHeight();
		coordinates.x = plant->GetX() + Random(width) - width / 2;
		// The butterfly assumes that the main part of a plant is in the upper half (below could just be the trunk)
		coordinates.y = plant->GetY() - Random(height)/2;
		attraction = plant;
		return true;
	}
	return false;
}

private func MissionComplete()
{
	if (!attraction) return _inherited(...);
	var wait = 20 + Random(80);
	// Slow animation speed
	fly_anim = PlayAnimation("Fly", 1, Anim_Linear(GetAnimationPosition(fly_anim), 0, fly_anim_len, 20, ANIM_Loop));
	ScheduleCall(this, "RegularSpeed", wait);
	SetCommand("Wait", nil,nil,nil,nil, wait);
}

private func MissionCompleteFailed()
{
	attraction = nil;
}

private func RegularSpeed()
{
	fly_anim = PlayAnimation("Fly", 1, Anim_Linear(GetAnimationPosition(fly_anim), 0, fly_anim_len, 10, ANIM_Loop));
}

// Hold the animation
private func SleepComplete()
{
	fly_anim = PlayAnimation("Fly", 1, Anim_Linear(GetAnimationPosition(fly_anim), 0, fly_anim_len/2, 10, ANIM_Hold));
	_inherited(...);
}

// Restart the animation
private func WakeUp()
{
	fly_anim = PlayAnimation("Fly", 1, Anim_Linear(GetAnimationPosition(fly_anim), 0, fly_anim_len, 10, ANIM_Loop));
	_inherited(...);
}

private func GetRestingPlace(proplist coordinates)
{
	// Try to rest in nearby grass
	for (var grass in FindObjects(Find_Distance(150), Find_ID(Grass), Sort_Distance()))
	{
		if (!Random(2)) continue;
		break;
	}
	if (grass)
	{
		coordinates.x = grass->GetX() + Random(4) - 2;
		coordinates.y = grass->GetY() + 2;
		return true;
	}
	return false;
}

private func Death(...)
{
	_inherited(...);
	fly_anim = PlayAnimation("Fly", 1, Anim_Linear(GetAnimationPosition(fly_anim), 0, fly_anim_len/2, 10, ANIM_Hold));
	SetAction("Dead");
}

/* Movement */

private func CheckTurn()
{
	if (GetEffect("Turning", this)) return;
	if (GetDir() == DIR_Left && GetXDir() > 0)
		AddEffect("Turning", this, 100, 1, this);
	if (GetDir() == DIR_Right && GetXDir() < 0)
		AddEffect("Turning", this, 100, 1, this);
}

private func FxTurningStart(object target, proplist effect, int temp)
{
	if (temp) return;
	effect.turn = 15;
	effect.step = 0;
	if (GetDir() == DIR_Right)
	{
		effect.turn = -15;
		effect.step = 360;
	}
	SetAction("SlowFly");
}

private func FxTurningTimer(object target, proplist effect)
{
	effect.step += effect.turn;
	if (effect.step == 0 || effect.step == 360) return FX_Execute_Kill;
	if (effect.step == 90)
	{
		effect.step = 285;
		SetDir(DIR_Right);
	}
	if (effect.step == 270)
	{
		effect.step = 75;
		SetDir(DIR_Left);
	}
	this.MeshTransformation = Trans_Mul(Trans_Rotate(270,1,1,1), Trans_Rotate(effect.step,0,0,1));
	return FX_OK;
}

private func FxTurningStop(object target, proplist effect, int reason, bool temp)
{
	if (temp) return;
	this.MeshTransformation = Trans_Rotate(270,1,1,1);
	SetAction("Fly");
}

/* Definition */

func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	props->Remove("ClrModulation"); // randomized in Initialize
	SaveScenarioObjectAction(props);
	return true;
}

local ActMap = {
	Fly = {
		Prototype = Action,
		Name = "Fly",
		Procedure = DFA_FLOAT,
		Speed = 100,
		Accel = 16,
		Decel = 16,
		Directions = 2,
		FlipDir = 1,
		Length = 1,
		Delay = 1,
		NextAction = "Fly",
		StartCall = "CheckTurn",
	},
	SlowFly = {
		Prototype = Action,
		Name = "SlowFly",
		Procedure = DFA_FLOAT,
		Speed = 30,
		Accel = 6,
		Decel = 6,
		Directions = 2,
		FlipDir = 1,
		Length = 1,
		Delay = 1,
		NextAction = "SlowFly",
	},
	Dead = {
		Prototype = Action,
		Name = "Dead",
		Directions = 2,
		FlipDir = 1,
		Length = 1,
		Delay = 1,
		NoOtherAction = 1,
	},
};

local Name = "Butterfly";
local MaxEnergy = 40000;
local MaxBreath = 125;
local Placement = 2;
local NoBurnDecay = true;
local BorderBound = C4D_Border_Sides | C4D_Border_Top | C4D_Border_Bottom;
local ContactCalls = true;

public func Definition(proplist def)
{
	def.PictureTransformation = Trans_Mul(Trans_Translate(400, 600, 0), Trans_Scale(1400), Trans_Rotate(20, 1, 0, 0), Trans_Rotate(40, 0, 1, 0));
}

