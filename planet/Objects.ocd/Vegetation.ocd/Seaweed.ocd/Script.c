/*-- Seaweed --*/

#include Library_Plant


func Place(int amount, proplist rectangle, proplist settings)
{
	var max_tries = 2 * amount;
	var loc_area = nil;
	if (rectangle) loc_area = Loc_InRect(rectangle);
	
	while ((amount > 0) && (--max_tries > 0))
	{
		var spot = FindLocation(Loc_Material("Water"), Loc_Wall(CNAT_Bottom), loc_area);
		if (!spot) continue;
		
		var f = CreateObjectAbove(this, spot.x, spot.y, NO_OWNER);
		--amount;
	}
	return true;
}

private func Initialize()
{
	SetAction("Sway");
	SetPhase(this.Action.Length); // ensure that not all seaweed are synced on scenario load
	return true;
}

private func Check()
{
	if(!GBackLiquid()) SetAction("Limp");
}

func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
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

