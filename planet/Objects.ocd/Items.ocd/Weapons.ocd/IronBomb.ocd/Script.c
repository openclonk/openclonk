/*
	IronBomb
	Author: Ringwaul

	Explodes after a short fuse.
*/
	

public func ControlUse(object clonk, int x, int y, bool box)
{
	// if already activated, nothing (so, throw)
	if(GetEffect("FuseBurn", this))
	{
		return false;
	}
	else
	{
		Fuse();
		return true;
	}
}

func Fuse()
{
	AddEffect("FuseBurn", this, 1,1, this);
}

func FxFuseBurnTimer(object bomb, int num, int timer)
{
	var i = 3;
	var x = +Sin(GetR(), i);
	var y = -Cos(GetR(), i);
	CreateParticle("EngineSmoke", x, y, x, y, RandomX(20,50), RGB(100,100,100));

	if(timer == 1) Sound("FuseLoop",nil,nil,nil,+1);
	if(timer >= 90)
	{
		Sound("FuseLoop",nil,nil,nil,-1);
		DoExplode();
		return -1;
	}
}

func DoExplode()
{
	var i = 23;
	while(i != 0)
	{
		var shrapnel = CreateObject(Shrapnel);
		shrapnel->SetVelocity(Random(359), RandomX(100,140));
		shrapnel->SetRDir(-30+ Random(61));
		CreateObject(BulletTrail)->Set(2,30,shrapnel);
		i--;
	}
	if(GBackLiquid())
		Sound("BlastLiquid2.ogg");
	else
		Sound("BlastMetal.ogg");
	CreateParticle("ExploSmoke", 0,0,0,0,390,RGBa(255,255,255,165));
	Explode(14);
}

protected func Hit() { Sound("RockHit*"); }

protected func Incineration() { Extinguish(); Fuse(); }

protected func RejectEntrance()
{
	return GetAction() == "Fuse" || GetAction() == "Ready";
}

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
local Rebuy = true;
