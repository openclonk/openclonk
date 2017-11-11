/*
	Mosquito
	Author: Clonkonaut

	A small annoying being.
*/

#include Library_InsectSwarm

local lib_swarm_standard = 20;
local lib_insect_max_dist = 50;
local lib_insect_nocturnal = true;

private func Initialize()
{
	SetAction("Fly");
	SetPhase(Random(3));

	_inherited(...);
}

private func Death()
{
	_inherited(...);
	RemoveObject();
}

private func MissionComplete()
{
	if (Time->IsDay()) return;

	// Mosquitos are very active
	MoveToTarget();
	// Little dipshits
}

private func Sleep()
{
	if (lib_insect_sleeping) return;

	if (lib_insect_going2sleep)
	{
		SetAction("Sleep");
		lib_insect_sleeping = true;
		return;
	}
	// One last trip, then become invisible
	MoveToTarget();
	lib_insect_going2sleep = true;
}

private func SleepComplete()
{
	SetAction("Sleep");
	_inherited(...);
}

private func WakeUp()
{
	SetAction("Fly");
	_inherited(...);
}

// Array: [object lovely_object, int x, int y]
local lovely_place;
// How many times I tried to get to the lovely place
local lovely_tries = 0;

// Mosquitos always look for a lovely place to hang out
private func GetAttraction(proplist coordinates)
{
	// GetAttraction will only be called for the swarm master, perfect to have just one being make sound
	if(!Random(30))
		Sound("Animals::MosquitoBuzz", nil,nil,nil,nil, 200);

	if (!lovely_place) lovely_place = CreateArray(3);
	if (!lovely_place[1])
	{
		if (lovely_tries < 35) lovely_tries++;
		if (lovely_tries > 15 + Random(30))
			// Should not be called too often.
			LookForLovelyPlace();
	}
	if (!lovely_place[1])
		return false; // No lovely place found. Default behaviour (just fly around)

	// Occasionally check if lovely place still qualifies
	if (!Random(100) && !CheckLovelyPlace())
		return false;

	// Lovely is an object (plant or corpse): Stay close
	if (lovely_place[0])
	{
		// Too far away from lovely place. Maybe I can't reach it?
		if (Distance(GetX(), GetY(), lovely_place[1], lovely_place[2]) > 50)
			lovely_tries++;
		coordinates.x = lovely_place[1] + Random(60) - 30;
		// This might fail with *very* high plants in respect of the next distance check. That's okay, I guess.
		coordinates.y = lovely_place[2] + Random(50) - 25;
	} else { // Lovely place is water: Fly around a bit
		// Too far away from lovely place. Maybe I can't reach it?
		if (Distance(GetX(), GetY(), lovely_place[1], lovely_place[2]) > 250)
			lovely_tries++;
		coordinates.x = lovely_place[1] + Random(300) - 150;
		coordinates.y = GetHorizonHeight(coordinates.x);
		if (!GBackLiquid(AbsX(coordinates.x), AbsY(coordinates.y))) // Stay on water
		{
			coordinates.x = lovely_place[1];
			coordinates.y = lovely_place[2];
		}
		coordinates.y -= 25 - Random(20);
	}

	// Tried to reach that place for far too long
	// Or maybe I've got enough of that place!
	if (lovely_tries >= 10 || !Random(1000))
		return RemoveLovelyPlace();

	return true;
}

private func LookForLovelyPlace()
{
	lovely_tries = 0;
	// Corpses are first priority places
	var corpse = FindObject(Find_Distance(200), Find_NoContainer(), Find_Category(C4D_Living), Find_Not(Find_OCF(OCF_Alive)), Find_Exclude(lovely_place[0]));
	if (corpse && Random(15))
	{
		lovely_place[0] = corpse;
		lovely_place[1] = corpse->GetX();
		lovely_place[2] = GetHorizonHeight(corpse->GetY()) - 30;
		return;
	}
	// Water is second best place
	var water = FindLocation(Loc_InRect(-100,-50,200,100), Loc_Material("Water"), Loc_MaxTries(80));
	if (water)
	{
		// Try to find the surface
		while (!GBackSky(water.x, water.y) && AbsY(water.y) > 0)
			water.y--;
		if (water.y > 0)
		{
			lovely_place[0] = nil;
			lovely_place[1] = water.x;
			lovely_place[2] = water.y;
			return;
		}
	}
	// Dense vegetation is third best place
	var plants = FindObjects(Find_Distance(200), Find_NoContainer(), Find_Func("IsPlant"), Find_Exclude(lovely_place[0]));
	if (GetLength(plants) >= 10)
	{
		var plant = plants[Random(GetLength(plants))];
		lovely_place[0] = plant;
		lovely_place[1] = plant->GetX();
		lovely_place[2] = GetHorizonHeight(plant->GetY()) - 30;
		return;
	}
}

private func CheckLovelyPlace()
{
	if (lovely_place[0]) // Corpse or plant
	{
		if (lovely_place[0]->~IsPlant())
			// Vegetation is not so dense anymore?
			if (lovely_place[0]->ObjectCount(Find_Distance(200), Find_NoContainer(), Find_Func("IsPlant"), Find_Exclude(lovely_place[0])) < 5)
				return RemoveLovelyPlace();
		// Place has moved?
		if (Distance(lovely_place[0]->GetX(), lovely_place[0]->GetY(), lovely_place[1], lovely_place[2]) > 100)
			return RemoveLovelyPlace();
	} else {
		// Water is gone?
		if (!GBackLiquid(AbsX(lovely_place[1]), AbsY(lovely_place[2])))
			return RemoveLovelyPlace();
	}
	return true;
}

private func RemoveLovelyPlace()
{
	lovely_tries = 0;
	// Don't delete lovely_place[0]
	// It will be ignored in future searches
	lovely_place[1] = nil;
	lovely_place[2] = nil;
	return false;
}

private func CheckTurn()
{
	if (GetXDir() < 0)
		SetDir(DIR_Left);
	if (GetXDir() > 0)
		SetDir(DIR_Right);
}

/*-- Saving --*/

public func SaveScenarioObject(proplist props)
{
	if (!inherited(props, ...)) return false;
	// Ignore some fast-changing stuff
	props->Remove("XDir");
	props->Remove("YDir");
	props->Remove("Command");
	return true;
}

/* Definition */

local ActMap = {
	Fly = {
		Prototype = Action,
		Name = "Fly",
		Procedure = DFA_FLOAT,
		Speed = 200,
		Accel = 32,
		Decel = 32,
		Directions = 2,
		FlipDir = 1,
		Length = 3,
		Delay = 2,
		X = 0,
		Y = 0,
		Wdt = 2,
		Hgt = 2,
		NextAction = "Fly",
		EndCall = "CheckTurn",
	},
	Sleep = {
		Prototype = Action,
		Name = "Sleep",
		Procedure = DFA_FLOAT,
		Speed = 0,
		Directions = 2,
		FlipDir = 1,
		Length = 1,
		Delay = 1,
		X = 6,
		Y = 0,
		Wdt = 2,
		Hgt = 2,
		NextAction = "Hold",
	},
};

local Name = "$Name$";
local MaxEnergy = 20000;
local MaxBreath = 250;
local Placement = 2;
local NoBurnDecay = true;
local BorderBound = C4D_Border_Sides | C4D_Border_Top | C4D_Border_Bottom;
local ContactCalls = true;