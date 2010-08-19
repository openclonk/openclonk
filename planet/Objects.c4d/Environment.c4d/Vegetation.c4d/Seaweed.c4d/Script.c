/*-- Seaweed --*/

/*
#include Library_Plant

private func SeedChance() { return 300; }
private func SeedAreaSize() { return 600; }
private func SeedAmount() { return 4; }
*/

private func Initialize()
{
	SetAction("Sway");
}

private func Check()
{
	if(!GBackLiquid()) SetAction("Limp");
}

func Definition(def) {
	SetProperty("Name", "$Name$", def);
	SetProperty("ActMap", {
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
},	}, def);
}