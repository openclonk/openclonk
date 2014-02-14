/*-- Seaweed --*/

#include Library_Plant

private func Initialize()
{
	SetAction("Sway");
}

private func Check()
{
	if(!GBackLiquid()) SetAction("Limp");
}

func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (GetPhase()) props->AddCall("Phase", this, "SetPhase", GetPhase()); // ensure that not all seaweed are synced on scenario load
	return true;
}



local Name = "$Name$";
local Placement = 1;
local ActMap = {
	Sway = {
		Prototype = Action,
		Name = "Sway",
		Procedure = DFA_NONE,
		Directions = 2,
		Length = 78,
		Delay = 2,
		X = 0,
		Y = 0,
		Wdt = 8,
		Hgt = 8,
		PhaseCall= "Check",
		NextAction = "Sway",
		Animation = "Sway",
	},
	Limp = {
		Prototype = Action,
		Name = "Limp",
		Procedure = DFA_NONE,
		Directions = 2,
		Length = 1,
		Delay = 1,
		X = 0,
		Y = 0,
		Wdt = 6,
		Hgt = 6,
		NextAction = "Limp",
		InLiquidAction = "Sway",
		Animation = "Limp",
	},
};

