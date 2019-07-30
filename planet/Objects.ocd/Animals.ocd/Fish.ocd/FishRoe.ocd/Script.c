/*
	Fish Roe
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
	
	var particles = 
	{
		Size = PV_Sin(PV_Step(PV_Random(1, 2)), PV_Random(0, 1), PV_Random(1, 2)),
		Attach = ATTACH_MoveRelative,
		Alpha = 100
	};
	particles = Particles_Colored(particles, HSL(Random(255), 100, 75));
	
	var amount = RandomX(5, 10);
	CreateParticle("SphereSpark", PV_Random(-3, 3), PV_Random(-3, 3), 0, 0, 0, particles, amount);
}

// Spawns a new fish. Called by the old fish.
func Hatch(id fish_id)
{
	for (var i = 0; i < 5; ++i)
		Bubble(1, RandomX(-5, 5), RandomX(-5, 5));
	var fish = CreateObjectAbove(fish_id, 0, 0, GetOwner());
	fish->SetCon(1);
	AddEffect("FastGrow", fish, 1, 1, nil, GetID());
	RemoveObject();
}

func FxFastGrowTimer(target, effect, time)
{
	target->DoCon(1);
	if (target->GetCon() >= 20)
		return FX_Execute_Kill;
}

func SwimAround()
{
	if (!GBackLiquid() || !Random(100)) return RemoveObject();
	SetSpeed(0, 0);
}

func Destruction()
{
	Bubble(5);
}

// todo: be able to serve caviar
// func NutritionalValue() { return 10; }
