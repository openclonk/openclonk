/**
	Flag Construction Preview Helper Arrow

	@author Sven
*/


protected func Initialize()
{
	this.Visibility = VIS_Owner;
	return;
}


/*-- Saving --*/

// The UI is not saved.
public func SaveScenarioObject() { return false; }


/*-- Properties --*/

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
