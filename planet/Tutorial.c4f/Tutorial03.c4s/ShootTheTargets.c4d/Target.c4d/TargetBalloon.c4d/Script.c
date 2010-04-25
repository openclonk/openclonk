/*-- Target Balloon --*/

local oldy;

func Definition(def) {
	SetProperty("Name", "$Name$", def);
}

protected func Initialize()
{
	oldy = GetY();
	SetAction("Float");
	AddEffect("Float",this,1,1,this);
}

func FxFloatTimer(object target, int num, int time)
{
	if(GetY() >= oldy) SetYDir(-1);
	if(GetY() < oldy - 10) SetYDir(1);
}

public func IsProjectileTarget(target,shooter)
{
	return 1;
}

public func OnProjectileHit()
{
	CreateObject(BurntBalloon,0,30);
	CastParticles("Air",20,5,0,-10,170,190,RGB(255,255,255),RGB(255,255,255));
	RemoveObject();
}

func FxFlyOffTimer(target, num, time)
{
	RemoveEffect("Float",this);
	if(GetYDir()>-30)
	{
		SetYDir(GetYDir()-1);
	}

	if(GetY()<0)
	{
		RemoveObject();
		return -1;
	}
}

func Definition(def) {
	SetProperty("ActMap", {

Float = {
	Prototype = Action,
	Name = "Float",
	Procedure = DFA_FLOAT,
	Directions = 1,
	FlipDir = 0,
	Length = 1,
	Delay = 1,
	X = 0,
	Y = 0,
	Wdt = 64,
	Hgt = 64,
	NextAction = "Float",
},
}, def);}