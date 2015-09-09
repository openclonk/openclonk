/*
	Firefly
	Author: Randrian, Nachtschatten (Clinfinity)

	A small glowing being, often encountered in groups.
*/
static const Firefly_MaxSpawnDistance = 15;
static const Firefly_MaxDistance = 40;
static const Firefly_ShynessDistance = 40;

local attracted_to;
local timer;

public func SpawnSwarm(object attracted_to, int size)
{
	if (!size) size = 10;
	var x = attracted_to->GetX();
	var y = attracted_to->GetY();
	for (var i = 0; i < size; i++)
	{
		var firefly = CreateObject(Firefly, RandomX(x - Firefly_MaxSpawnDistance, x + Firefly_MaxSpawnDistance), RandomX(y - Firefly_MaxSpawnDistance, y + Firefly_MaxSpawnDistance), NO_OWNER);
		firefly.attracted_to = attracted_to;
	}
}


private func Flying() {
	var xdir, ydir;

	timer += Random(2);
	var angle = timer*180/10;
	SetObjDrawTransform(900+Sin(angle,100), 0, 0, 0, 900+Sin(angle, 100), 0, 1);
	SetLightColor(RGB(200,255,150+Sin(angle,10)));
	
	var away_from = FindObject(Find_Distance(Firefly_ShynessDistance), Find_Category(C4D_Object), Find_OCF(OCF_HitSpeed1), Find_NoContainer());
	if (away_from)
	{
		xdir = BoundBy(GetX() - away_from->GetX(), -1, 1);
		ydir = BoundBy(GetY() - away_from->GetY(), -1, 1);
		if(xdir == 0) xdir = Random(2) * 2 - 1;
		if(ydir == 0) ydir = Random(2) * 2 - 1;
		xdir = RandomX(5 * xdir, 10 * xdir);
		ydir = RandomX(5 * ydir, 10 * ydir);
		// No check for liquids here, you can scare fireflies into those ;)
		SetSpeed(xdir, ydir);
	}
	else
	{
		if (Random(4)) return;
		
		if (attracted_to != 0 && ObjectDistance(attracted_to) > Firefly_MaxDistance)
		{
			xdir = BoundBy(attracted_to->GetX() - GetX(), -1, 1);
			ydir = BoundBy(attracted_to->GetY() - GetY(), -1, 1);
			xdir = RandomX(xdir, 6 * xdir);
			ydir = RandomX(ydir, 6 * ydir);
		}
		else
		{
			xdir = Random(120) - 60;
			ydir = Random(80) - 40;
		}

		if (GBackLiquid(xdir, ydir))
		{
			SetSpeed(0, 0);
		}
		else
		{
			xdir *= 10;
			ydir *= 10;
			while(GBackSemiSolid(0, ydir)) ydir-=1;
			SetCommand("MoveTo", 0, GetX()+xdir, GetY()+ydir);
			if(Random(10)==0 && attracted_to)
				SetCommand("MoveTo", attracted_to);
		}
	}
}

private func Check()
{
	// Buried or in water: Instant death
	if (GBackSemiSolid())
	{
		Death();
	}
}

public func Initialize()
{
	SetAction("Fly");
	
	SetGraphics("", GetID(), 1, 1, 0, GFX_BLIT_Additive);
	SetClrModulation(RGBa(200,255,100,50),1);
	SetGraphics("", GetID(), 2, 1, 0, GFX_BLIT_Additive);
	SetClrModulation(RGBa(200,255,0,255),2);

	SetObjDrawTransform(300, 0, 0, 0, 300, 0, 2);
	
	SetLightRange(5,30);
	SetLightColor(RGB(200,255,100));
}

public func CatchBlow() { RemoveObject(); }
public func Damage() { RemoveObject(); }
public func Death() { RemoveObject(); }

local ActMap = {
	Fly = {
		Prototype = Action,
		Name = "Fly",
		Procedure = DFA_FLOAT,
		Speed = 30,
		Accel = 5,
		Decel = 5,
		Directions = 2,
		Length = 1,
		Delay = 1,
		NextAction = "Fly",
		PhaseCall = "Flying",
		EndCall = "Check",
	},
};

local Name = "$Name$";
local MaxEnergy = 40000;
local MaxBreath = 125;
local Placement = 2;
local NoBurnDecay = 1;
