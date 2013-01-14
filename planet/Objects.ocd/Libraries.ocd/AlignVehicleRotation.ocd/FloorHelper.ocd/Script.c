protected func Initialize()
{
	SetAction("Floor");
}

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