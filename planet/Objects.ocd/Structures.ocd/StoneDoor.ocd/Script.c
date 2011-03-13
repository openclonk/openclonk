/*-- Stone Door --*/

protected func Initialize()
{
	SetAction("Door");
	return;
}

public func OpenGateDoor()
{
	AddEffect("IntMoveGateUp", this, 100, 1, this);
	Sound("GateMove");
	return;
}

public func CloseGateDoor()
{
	AddEffect("IntMoveGateDown", this, 100, 1, this);
	Sound("GateMove");
	return;
}

protected func FxIntMoveGateUpTimer(object target)
{
	if (GBackSolid(0, -20))
	{
		Sound("GateHit.ogg");
		SetYDir(0);
		return -1;
	}
	
	SetYDir(-5);
	return 1;
}

protected func FxIntMoveGateDownTimer(object target)
{
	if (GBackSolid(0, 19))
	{
		Sound("GateHit.ogg");
		SetYDir(0);
		return -1;
	}
	
	SetYDir(5);
	return 1;
}	

func Definition(def) 
{
	SetProperty("ActMap", {
		Door = {
			Prototype = Action,
			Name = "Door",
			Procedure = DFA_FLOAT,
			Length = 1,
			Delay = 1,
			X = 0,
			Y = 0,
			Wdt = 10,
			Hgt = 40,
			NextAction = "Door",
		},
	}, def);
	SetProperty("Name", "$Name$", def);
}
