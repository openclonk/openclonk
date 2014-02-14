/*-- Flag construction preview helper arrow --*/

protected func Initialize()
{
	this["Visibility"] = VIS_Owner;
	return;
}

// UI not saved.
func SaveScenarioObject() { return false; }

/*-- Proplist --*/

local ActMap = {
		Show = {
			Prototype = Action,
			Name = "Show",
			Procedure = DFA_ATTACH,
			Length = 1,
			Delay = 0,
			X = 0,
			Y = 0,
			Wdt = 40,
			Hgt = 20,
			NextAction = "Show",
		},
};
local Name = "$Name$";
