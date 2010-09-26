/*-- Castle Door --*/

protected func Initialize()
{
	SetAction("Door");
}

public func OpenGateDoor()
{
	//Non-working vertices...
	AddEffect("StopHit",this,1,1,this);
	if (GetContact(CNAT_Top))
	{
		Sound("GateStuck");
		return;
	}
	if (!GBackSolid(0,-21)) SetYDir(-5);
	Sound("GateMove");
}

func FxStopHitTimer(object door, int num, int timer)
{
	if (GBackSolid(0,-21) || GBackSolid(0,21))
	{
		Hit();
		return -1;
	}
}

public func CloseGateDoor()
{
	if (GetContact(CNAT_Bottom) || GBackSolid(0,21))
	{
		Hit();
		return;
	}
	AddEffect("StopHit",this,1,1,this);
	SetYDir(5);
	Sound("GateMove");
}

public func Hit()
{
	Sound("GateHit.ogg");
	SetSpeed();
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
