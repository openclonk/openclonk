/**
	Flag Library: Construction Preview Arrow

	@author Sven
*/


/*-- Saving --*/

// The UI is not saved.
public func SaveScenarioObject() { return false; }


/*-- Properties --*/

local Visibility = VIS_Owner;

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
