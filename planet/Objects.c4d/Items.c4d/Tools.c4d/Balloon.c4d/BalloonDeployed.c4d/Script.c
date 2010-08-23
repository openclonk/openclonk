/*-- Balloon - Delpoyed --*/

local rider;
local parent;
local idir,dir;

protected func Initialize()
{
	idir = 0;
	dir = 0;
	SetAction("Inflate");
	AddEffect("Float",this,1,1,this);

	//Special Effects
	var i = 0;
	while(i <= 7)
	{
		CreateParticle("Air", 0,16,-3 + Random(7),Random(2),RandomX(70,150),RGB(255,255,255), this);
		++i;
	}
}

public func ControlUseStart(object clonk, int ix, int iy)
{
	//reroute Control commands to clonk's inventory
	return false;
}

private func Deflate()
{
	if(GetAction() != "Deflate") SetAction("Deflate");
	Schedule("Pack()",20); //EndCall doesn't work. >:(
}

private func DeflateEffect()
{
	CreateParticle("Air",0,16,0,0,GetActTime()*3,RGB(255,255,255));
}

private func Pack()
{
	RemoveEffect("NoDrop",parent);
	RemoveObject();
	rider->SetAction("Jump");
	rider->SetSpeed(GetXDir(),GetYDir());
}

func HoldingEnabled() { return true; }

func ControlLeft()
{
	idir = -1;
	return true;
}

func ControlRight()
{
	idir = 1;
	return true;
}

func ControlStop()
{
	idir = 0;
	return true;
}

func ControlJump()
{
	Deflate();
}


public func IsProjectileTarget(target,shooter)
{
	return 1;
}

public func OnProjectileHit()
{
	CastParticles("Air",20,5,0,-10,170,190,RGB(255,255,255),RGB(255,255,255));
	if(rider!=nil) rider->SetAction("Tumble");
	rider->SetSpeed(GetXDir(),GetYDir());
	parent->RemoveObject();
	RemoveObject();
}

private func FxFloatTimer(object target, int num, int time)
{
	var speed = 7;
	if(GetYDir() > speed) SetYDir(GetYDir() - 1);
	if(GetYDir() < speed) SetYDir(GetYDir() + 1);
	if(GetXDir() > speed * 3) SetXDir(GetXDir()-1);
	if(GetXDir() < -speed * 3) SetXDir(GetXDir()+1);

	//Control
	SetXDir(GetXDir() + idir);
	Message(Format("%d",idir));

	if(GetContact(-1)) //Has any vertex hit? Then deflate.
	{
		Deflate();
		return -1;
	}
	if(GBackSolid(0,50) || GBackLiquid(0,50))
	{
		Deflate();
		return 1;
	}
}

func Definition(def) {
	SetProperty("ActMap", {

Float = {
	Prototype = Action,
	Name = "Float",
	Procedure = DFA_FLOAT,
	Directions = 1,
	Length = 144,
	Delay = 1,
	NextAction = "Float",
	Animation = "Fly",
},

Inflate = {
	Prototype = Action,
	Name = "Inflate",
	Procedure = DFA_FLOAT,
	Directions = 1,
	Length = 20,
	Delay = 1,
	NextAction = "Float",
	Animation = "Inflate",
},

Deflate = {
	Prototype = Action,
	Name = "Deflate",
	Procedure = DFA_FLOAT,
	Directions = 1,
	Length = 20,
	Delay = 1,
	PhaseCall = "DeflateEffect",
	EndCall = "Pack",
	Animation = "Deflate",
},
}, def);
	SetProperty("Name", "$Name$",def);
}