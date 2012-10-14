/*-- Lichen --*/

#include Library_Plant

local grow_stage;

private func SeedChance() { return 1000; }

private func IsCrop() { return true; }
private func SickleHarvesting() { return false; }

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
	var moss = CreateObject(Moss, 0, GetObjHeight()/2, NO_OWNER);
	clonk->Collect(moss);
	CreateParticle("Lichen", 0,0, 10, -2, 75, RGBa(128,128,128,255));
	CreateParticle("Lichen", 0,0,-10, -2, 75, RGBa(128,128,128,255));

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

local Name = "$Name$";
local Description = "$Description$";
local BlastIncinerate = 1;
local ContactIncinerate = 3;
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