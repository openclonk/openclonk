/**
	CoralSeed
*/

local Name = "$Name$";
local Description = "$Description$";

local ActMap = {
Swim = {
	Prototype = Action,
	Name = "Swim",
	Procedure = DFA_SWIM,
	Speed = 100,
	Accel = 16,
	Decel = 16,
	Length = 1,
	Delay = 0,
	FacetBase = 1,
	NextAction = "Swim"
}
};

func Initialize()
{
	SetCategory(C4D_None);
	
	AddTimer("SwimAround", 15);
	SetAction("Swim");
	SetComDir(COMD_None);
}


func SwimAround()
{
	if (!GBackLiquid() || !Random(20)) return RemoveObject();
	
	SetVelocity(Random(360), 1);
}

// for fishes
func NutritionalValue() { return 1; }
