/*
	Wheat stalk
	Author: Clonkonaut
*/

local mother;

protected func Construction()
{
	// Editable ActMap
	ActMap = { Prototype = this.Prototype.ActMap };
	AddTimer("AdjustGrowth");
}

// The stalk will adjust its growth to the growth of the mother plant
public func SetMother(object wheat)
{
	mother = wheat;
}
// Timer
public func AdjustGrowth()
{
	if (!mother) return;
	if (GetCon() != mother->GetCon())
		SetCon(mother->GetCon());
}

// Sets the actions' delays
// Of course this does mean that: 0 = no movement, 1 = fastest movement, >1 = movements slows down
public func SetSwingSpeed(int delay)
{
	ActMap["Swing"] = { Prototype = ActMap["Swing"], Delay = delay };
	ActMap["Swing2"] = { Prototype = ActMap["Swing2"], Delay = delay };
	// Restart action
	var phase = GetPhase();
	SetAction(GetAction());
	SetPhase(phase);
}

// Only save main wheat object
func SaveScenarioObject() { return false; }

local ActMap = {
		Swing = {
			Prototype = Action,
			Name = "Swing",
			Procedure = DFA_NONE,
			Directions = 2,
			FlipDir = 1,
			X = 0,
			Y = 0,
			Wdt = 8,
			Hgt = 20,
			Delay = 1,
			Length = 90,
			NextAction = "Swing2"
		},
		Swing2 = {
			Prototype = Action,
			Name = "Swing",
			Procedure = DFA_NONE,
			Directions = 2,
			FlipDir = 1,
			X = 0,
			Y = 0,
			Wdt = 8,
			Hgt = 20,
			Delay = 1,
			Length = 90,
			Reverse = 1,
			NextAction = "Swing"
		}
};