/*
	IronBomb
	Author: Ringwaul, Clonkonaut

	Explodes after a short fuse. Explodes on contact if shot by the grenade launcher
*/

local armed; // If true, explodes on contact

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

func Fuse(bool explode_on_hit)
{
	armed = explode_on_hit;
	AddEffect("FuseBurn", this, 1,1, this);
}

func FxFuseBurnTimer(object bomb, int num, int timer)
{
	var i = 3;
	var x = +Sin(GetR(), i);
	var y = -Cos(GetR(), i);
	CreateParticle("Smoke", x, y, x, y, PV_Random(18, 36), Particles_Smoke(), 2);

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
		shrapnel->Launch(GetController());
		CreateObject(BulletTrail)->Set(2,30,shrapnel);
		i--;
	}
	if(GBackLiquid())
		Sound("BlastLiquid2");
	else
		Sound("BlastMetal");
	CreateParticle("Smoke", PV_Random(-30, 30), PV_Random(-30, 30), 0, 0, PV_Random(40, 60), Particles_Smoke(), 60);
	Explode(30);
}

protected func Hit(x, y)
{
	if (armed) return DoExplode();
	StonyObjectHit(x,y);
}

protected func Incineration() { Extinguish(); Fuse(); }

protected func RejectEntrance()
{
	return GetAction() == "Fuse" || GetAction() == "Ready";
}

public func IsWeapon() { return true; }
public func IsArmoryProduct() { return true; }
public func IsGrenadeLauncherAmmo() { return true; }

local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local Collectible = 1;
local Rebuy = true;
local BlastIncinerate = 1;
local ContactIncinerate = 1;
