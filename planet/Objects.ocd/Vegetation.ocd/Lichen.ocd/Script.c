/*-- Lichen --*/

#include Library_Plant
#include Library_Crop

local grow_stage;

local plant_seed_chance = 10;
local plant_seed_offset = 10;

private func SickleHarvesting() { return false; }

protected func Construction()
{
	var graphic = Random(6);
	if (graphic)
		SetGraphics(Format("%d",graphic));
	_inherited(...);

	if (GetCon() < 100) SetCon(100);
}

protected func Initialize()
{
	SetAction("Grow");
}

protected func Grow()
{
	grow_stage++;
}

public func Harvest(object clonk)
{
	var moss = CreateObjectAbove(Moss, 0, GetObjHeight()/2, NO_OWNER);
	clonk->Collect(moss);
	var particles = 
	{
		Size = PV_Random(3, 7),
		Alpha = PV_KeyFrames(0, 0, 255, 900, 255, 1000, 0),
		ForceY = PV_Gravity(40),
		DampingX = 900, DampingY = 900,
		CollisionVertex = 750,
		OnCollision = PC_Stop(),
		Rotation = PV_Direction(PV_Random(900, 1100)),
		Phase = PV_Random(0, 1)
	};
	CreateParticle("Lichen", PV_Random(-5, 5), PV_Random(-5, 5), PV_Random(-30, 30), PV_Random(-30, 30), PV_Random(36, 36 * 4), particles, 40);

	if (grow_stage)
	{
		if (GetAction() != "Grow") SetAction("Grow");
		grow_stage--;
		SetPhase(grow_stage);
	}
	else
		RemoveObject();
	return true;
}

func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (GetAction() != "Grow" || GetPhase()) SaveScenarioObjectAction(props);
	return true;
}

local Name = "$Name$";
local Description = "$Description$";
local BlastIncinerate = 1;
local ContactIncinerate = 3;
local Components = {Moss = 4};

local ActMap = {
	Grow = {
		Prototype = Action,
		Name = "Grow",
		Procedure = DFA_NONE,
		Length = 4,
		Delay = 2500,
		X = 0,
		Y = 0,
		Wdt = 25,
		Hgt = 25,
		NextAction = "Grown",
		PhaseCall = "Grow"
	},
	Grown = {
		Prototype = Action,
		Name = "Grown",
		Procedure = DFA_NONE,
		Length = 1,
		Delay = 0,
		X = 75,
		Y = 0,
		Wdt = 25,
		Hgt = 25,
		NextAction = "Grown"
	}
};
