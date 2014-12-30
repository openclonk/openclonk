/**
	Wheel
	Invisible helper object. Takes care of collisions.
*/


protected func Initialize()
{
	return;
}

public func AttachTargetLost()
{
	// Remove this helper object when the generator is lost.
	return RemoveObject();
}

public func HasStopped()
{
	return !GetRDir(1000);
}

public func SetParent(object parent, int con)
{
	con = con ?? 100;
	SetCon(con);
	SetAction("Turn", parent);
	return;
}

// Don't duplicate on scenario load.
public func SaveScenarioObject() { return false; }


/*-- Proplist --*/

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