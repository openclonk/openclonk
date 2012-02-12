/*-- Arrow target --*/

func Definition(def) {
	SetProperty("Name", "$Name$", def);
}

protected func Initialize()
{
	SetAction("Attach");
	PlayAnimation("idle", 1, Anim_Linear(0, 0, GetAnimationLength("idle"), 1000, ANIM_Loop), Anim_Const(1000));
}

public func IsProjectileTarget(target,shooter)
{
	return 1;
}

public func OnProjectileHit()
{
	//Makes balloon fly away
	if(GetActionTarget()!=nil)
	{
	GetActionTarget()->AddEffect("FlyOff",GetActionTarget(),1,1,GetActionTarget());
	}

	Burst();
	return 1;
}

public func Burst()
{
	RemoveObject();
	CastParticles("Straw",130,30,0,-3,30,40,RGB(255,255,255),RGB(255,255,255));
}

public func Hit()
{
	Burst();
}

protected func Tumble()
{
	SetRDir(-4+Random(8));
}

func Definition(def) {
	SetProperty("ActMap", {

Fall = {
	Prototype = Action,
	Name = "Fall",
	Procedure = DFA_FLIGHT,
	Speed = 200,
	Accel = 16,
	Directions = 1,
	FlipDir = 0,
	Length = 1,
	Delay = 1,
	X = 0,
	Y = 0,
	Wdt = 25,
	Hgt = 25,
	NextAction = "Fall",
	StartCall = "Tumble",
},

Attach = {
	Prototype = Action,
	Name = "Attach",
	Procedure = DFA_ATTACH,
	Directions = 1,
	FlipDir = 0,
	Length = 40,
	Delay = 15,
	X = 0,
	Y = 0,
	Wdt = 25,
	Hgt = 25,
	NextAction = "Attach",
//	Animation = "idle",
},

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
	Wdt = 25,
	Hgt = 25,
	NextAction = "FLOAT",
},
}, def);}
