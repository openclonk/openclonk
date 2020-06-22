/*-- Switch Handle --*/

func SetSwitch(object s)
{
	return SetAction("Attach", s);
}

func SaveScenarioObject() {}

local ActMap = {
	Attach = {
		Prototype = Action,
		Name = "Attach",
		Procedure = DFA_ATTACH,
		FacetBase = 1,
		Length = 1,
		Delay = 0,
		NextAction = "Hold",
	},
};

local Name = "$Name$";
local Description = "$Description$";
local Plane = 269;
local Components = { Metal = 1 };
