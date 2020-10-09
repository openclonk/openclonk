/*
	Zap

	Author: Clonkonaut

	Protects its hive!
*/

#include Library_InsectSwarm

local lib_swarm_standard = 5;
local swarm_enraged = 20;
local lib_swarm_density = 5;
local lib_insect_max_dist = 100;

local home;
local enraged;
local enrage_target;
local sting_damage = 2;

// Called by the zaphive
public func SetHome(object my_castle)
{
	home = my_castle;
	this->CreateSwarm(lib_swarm_standard);
}

// Called by the zaphive
public func SetEnraged()
{
	this->CreateSwarm(swarm_enraged);
	SwarmCall("SetEnragedSwarm");
}

// Called by swarm helper
public func SetEnragedSwarm()
{
	enraged = true;
	SetAction("Attack");
}

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
	if (enraged)
		MoveToTarget();
	else
		_inherited(...);
}

private func Sleep()
{
	if (enraged) return MoveToTarget();
	if (lib_insect_sleeping) return;

	if (lib_insect_going2sleep)
	{
		SetAction("Sleep");
		lib_insect_sleeping = true;
		return;
	}
	// One last trip, then become invisible
	MoveToTarget();
	// Insect might have been removed.
	if (this)
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

private func GetAttraction(proplist coordinates)
{
	if (enraged) return Enrage(coordinates);
	if (!home)
	{
		HomeIsLost();
		return false;
	}
	// GetAttraction will only be called for the swarm master, perfect to have just one being make sound
	if (!Random(20))
		Sound("Animals::Zap::Zap?", nil, nil, nil, nil, 200, Random(100));

	coordinates.x = home->GetX() + Random(20)-10;
	coordinates.y = home->GetY() + Random(20)-10;
	return true;
}

private func HomeIsLost()
{
	if (!Random(2)) Kill();
}

private func Enrage(proplist coordinates)
{
	if (!enrage_target)
		CheckTarget();
	if (!this)
		return false;
	if (!enrage_target)
		return false;
	if (ObjectDistance(enrage_target) < 10)
		return Sting();

	if (!(enrage_target->GetAlive())) return Kill();
	if (enrage_target->Contained())
	{
		if (!Random(25)) return Kill();
		return false;
	}
	if (!Random(50)) return Kill();
	if (!GBackSky() && !Random(25)) return Kill();

	coordinates.x = enrage_target->GetX();
	coordinates.y = enrage_target->GetY();
	return true;
}

private func Sting()
{
	if (!enrage_target) return false;

	Punch(enrage_target, sting_damage);
	Kill();
	return true;
}

// Look for a target to attack
private func CheckTarget()
{
	var clonk = FindObject(Find_Distance(200), Find_OCF(OCF_CrewMember), Find_OCF(OCF_Alive), Find_NoContainer(), Find_PathFree(), Sort_Distance());
	if (clonk)
	{
		SwarmCall("DoAttack", clonk);
		return;
	}
	if (!Random(10)) Kill();
}

public func DoAttack(object to_kill)
{
	enrage_target = to_kill;
}

private func CheckTurn()
{
	if (GetXDir() < 0)
		SetDir(DIR_Left);
	if (GetXDir() > 0)
		SetDir(DIR_Right);
}

private func AngryBuzz()
{
	Sound("Animals::Zap::Zap?", {pitch = -Random(100)});
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
		Speed = 100,
		Accel = 5,
		Decel = 5,
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
	Attack = {
		Prototype = Action,
		Name = "Attack",
		Procedure = DFA_FLOAT,
		Speed = 200,
		Accel = 30,
		Decel = 30,
		Directions = 2,
		FlipDir = 1,
		Length = 3,
		Delay = 2,
		X = 0,
		Y = 0,
		Wdt = 2,
		Hgt = 2,
		NextAction = "Attack",
		StartCall = "AngryBuzz",
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
local MaxEnergy = 30000;
local MaxBreath = 250;
local Placement = 2;
local NoBurnDecay = true;
local BorderBound = C4D_Border_Sides | C4D_Border_Top | C4D_Border_Bottom;
local ContactCalls = true;
