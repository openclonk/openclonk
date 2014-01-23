/**
	Wheel
	Invisible helper object. Takes care of collisions.

*/

local ActMap = {
	Turn = {
		Prototype = Action,
		Name = "Turn",
		Procedure = DFA_ATTACH,
		Length = 1,
		Delay = 0,
		FacetBase=1,
		NextAction = "Hold",
	}
};

func Initialize()
{
	
}

func AttachTargetLost()
{
	return RemoveObject();
}

func HasStopped()
{
	return !GetRDir(1000);
}

func Set(to, con)
{
	con = con ?? 100;
	SetCon(con);
	SetAction("Turn", to);
}

// Don't duplicate on scenario load
func SaveScenarioObject() { return false; }