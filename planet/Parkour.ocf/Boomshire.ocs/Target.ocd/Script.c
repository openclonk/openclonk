/*-- Arrow target --*/

local gate;

protected func Initialize()
{
	SetAction("Attach");
	PlayAnimation("idle", 1, Anim_Linear(0, 0, GetAnimationLength("idle"), 1000, ANIM_Loop));
}

public func IsProjectileTarget(object projectile, object shooter)
{
	return true;
}
public func SetGate(object g)
{
	gate = g;
}

public func OnProjectileHit()
{
	//Makes balloon fly away
	if (GetActionTarget()!=nil)
	{
	GetActionTarget()->AddEffect("FlyOff",GetActionTarget(),1, 1, GetActionTarget());
	}

	Burst();
	return 1;
}

public func Burst()
{
	DrawParticleLine("Straw", 0, 0, AbsX(gate->GetX()), AbsY(gate->GetY()), 6, PV_Random(-5, 5), PV_Random(-5, 0), PV_Random(30, 60), Particles_Straw());
	CreateParticle("Straw", 0, 0, PV_Random(-30, 30), PV_Random(-30, 30), PV_Random(30, 120), Particles_Straw(), 200);
	gate->OpenDoor();
	RemoveObject();
}

public func Hit()
{
	Burst();
}

protected func Tumble()
{
	SetRDir(-4 + Random(8));
}

func Definition(def) {
	SetProperty("Name", "$Name$", def);
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
