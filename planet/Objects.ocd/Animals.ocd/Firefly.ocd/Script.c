/**
	Firefly
	A small glowing being, often encountered in groups.
	
	@authors Randrian, Nachtschatten
*/

#include Library_InsectSwarm

local lib_swarm_standard = 10;
local lib_insect_max_dist = 40;
local lib_swarm_density = 8;
local lib_insect_nocturnal = true;

public func Initialize()
{
	SetAction("Fly");
	SetGraphics("", GetID(), 1, 1, nil, GFX_BLIT_Additive);
	SetClrModulation(RGBa(200, 255, 100, 50), 1);
	SetGraphics("", GetID(), 2, 1, nil, GFX_BLIT_Additive);
	SetClrModulation(RGBa(200, 255, 0, 255), 2);
	SetObjDrawTransform(300, 0, 0, 0, 300, 0, 2);
	
	// Fireflies shine a bit of light.
	SetLightRange(5, 30);
	SetLightColor(RGB(200, 255, 100));
	return _inherited(...);
}

public func Death()
{
	_inherited(...);
	return RemoveObject();
}

public func CatchBlow()
{
	_inherited(...);
	return RemoveObject();
}

public func Damage()
{
	_inherited(...);
	return RemoveObject();
}

// Attracted to trees.
private func GetAttraction(proplist coordinates)
{
	// Sometimes I don't want to fly to a plant.
	if (!Random(7)) return false;
	for (var plant in FindObjects(Find_Distance(150), Find_Func("IsTree"), Sort_Distance()))
	{
		if (!Random(4))
			continue;
		if (ObjectDistance(plant) < 20) // Too close.
			continue;
		if (plant->GetCon() < 30) // Too small.
			continue;
		if (plant->GBackSemiSolid()) // Under water or covered with solid material.
			continue;
		var width = plant->GetObjWidth();
		var height = plant->GetObjHeight();
		coordinates.x = plant->GetX() + Random(width) - width / 2;
		// The firefly assumes that the main part of a plant is in the upper half (below could just be the trunk).
		coordinates.y = plant->GetY() - Random(height) / 2;
		return true;
	}
	return false;
}

private func MissionComplete()
{
	// Fireflies are active.
	MoveToTarget();
	return;
}

// No sleeping.
private func Sleep()
{
	this.Visibility = VIS_None;
	_inherited(...);
}


private func WakeUp()
{
	this.Visibility = VIS_All;
	_inherited(...);
}

// Action end call.
private func Check()
{
	// Buried or in water: Instant death
	if (GBackSemiSolid())
	{
		Kill();
	}
	return;
}


/*-- Properties --*/

local ActMap = {
	Fly = {
		Prototype = Action,
		Name = "Fly",
		Procedure = DFA_FLOAT,
		Speed = 30,
		Accel = 5,
		Decel = 5,
		Directions = 2,
		Length = 1,
		Delay = 1,
		NextAction = "Fly",
		EndCall = "Check",
	},
};

local Name = "$Name$";
local MaxEnergy = 40000;
local MaxBreath = 125;
local Placement = 2;
local NoBurnDecay = true;
local BorderBound = C4D_Border_Sides | C4D_Border_Top | C4D_Border_Bottom;
local ContactCalls = true;
