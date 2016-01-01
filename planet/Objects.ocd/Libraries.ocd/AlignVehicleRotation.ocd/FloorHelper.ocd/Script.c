protected func Initialize()
{
	SetAction("Floor");
}

local Plane = 300;
local BorderBound = C4D_Border_Sides | C4D_Border_Top | C4D_Border_Bottom;
local ActMap = {
		Floor = {
			Prototype = Action,
			Name = "Floor",
			Procedure = DFA_FLOAT,
			Directions = 1,
			X = 0,
			Y = 0,
			Wdt = 0,
			Hgt = 0,
			NextAction = "Floor",
		},
};