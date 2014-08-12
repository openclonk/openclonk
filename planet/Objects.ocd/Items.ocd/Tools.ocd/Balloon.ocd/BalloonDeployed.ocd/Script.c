/*-- Balloon - Deployed --*/

local rider;
local parent;
local idir;

protected func Initialize()
{
	idir = 0;
	SetAction("Inflate");
	SetComDir(COMD_None);
	AddEffect("Float",this,1,1,this);

	//Special Effects
	CreateParticle("Air", PV_Random(-1, 1), PV_Random(15, 17), PV_Random(-3, 3), PV_Random(0, 2), 18, Particles_Air(), 20);
}

private func Deflate()
{
	if(GetAction() != "Deflate")
	{
		SetAction("Deflate");
		SetComDir(COMD_None);
	}
	Schedule(this,"Pack()",20); //EndCall doesn't work. >:(
}

private func DeflateEffect()
{
	var act_time = GetActTime();
	CreateParticle("Air", PV_Random(-1, 1), PV_Random(-1, 5), PV_Random(-act_time, act_time), PV_Random(-act_time, act_time), 18, Particles_Air(), act_time);
}

private func Pack()
{
	RemoveEffect("NoDrop",parent);
	rider->SetAction("Jump");
	rider->SetSpeed(GetXDir(),GetYDir());
	rider->SetComDir(COMD_Down);
	RemoveObject();
}

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
	//Pop!
	CreateParticle("Air", 0, -10, PV_Random(-10, 10), PV_Random(-10, 10), 10, Particles_Air(), 30);
	Sound("BalloonPop");
	if (rider)
	{
		rider->SetAction("Tumble");
		rider->SetSpeed(GetXDir(),GetYDir());
	}
	parent->RemoveObject();
	RemoveObject();
}

private func FxFloatTimer(object target, effect, int time)
{
	var speed = 7;
	if(GetYDir() > speed) SetYDir(GetYDir() - 1);
	if(GetYDir() < speed) SetYDir(GetYDir() + 1);
	if(GetXDir() > speed * 3) SetXDir(GetXDir()-1);
	if(GetXDir() < -speed * 3) SetXDir(GetXDir()+1);

	//Control
	SetXDir(GetXDir() + idir);
	//Message(Format("%d",idir));

	if(GetContact(-1) & CNAT_Bottom || Stuck()) //Has a bottom vertex hit? Is the balloon stuck in material? Then deflate.
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

// Could store and restore the deployed balloon, but all the
// dependencies to be set when recreating this mid-animation
// will probably cause more upwards incompatibilities than benefit
func SaveScenarioObject() { return false; }

local ActMap = {

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
};
local Name = "$Name$";
