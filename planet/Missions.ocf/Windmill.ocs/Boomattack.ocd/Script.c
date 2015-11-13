/*--
	Boom attack
	Authors: Randrian, Newton

	An evil rocket which is hungry on the destruction of windmills
--*/

local fuel;

protected func Construction()
{
	//flight length
	fuel=1000;
}


protected func FxFlightTimer(object pTarget, effect, int iEffectTime)
{
	if(fuel<=0)
	{
		DoFireworks();
		return;
	}

	var ignition = iEffectTime % 10;
	
	if(!ignition)
	{
		var angle = GetR()+RandomX(-3,3);
		SetXDir(Sin(angle,100), 100);
		SetYDir(-Cos(angle,100), 100);
		SetR(angle);
	}
	
	if(GetAction() != "Fly")
	{
		SetAction("Fly");
		SetComDir(COMD_None);
	}
	
	var x = -Sin(GetR(), 15);
	var y = +Cos(GetR(), 15);

	var xdir = GetXDir() / 2;
	var ydir = GetYDir() / 2;
	CreateParticle("FireDense", x, y, PV_Random(xdir - 4, xdir + 4), PV_Random(ydir - 4, ydir + 4), PV_Random(16, 38), Particles_Thrust(), 5);
	
	
	fuel--;
}

public func IsProjectileTarget(target,shooter)
{
	if(target->GetID() == GetID())
	{
		return false;
	}
	return true;
}

public func OnProjectileHit(object shot)
{
	var gol = FindObject(Find_ID(Goal_SaveTheWindmills));
	if(gol)	gol->IncShotScore(shot->GetController());
	DoFireworks();
	return 1;
}

/* Contact */

protected func ContactBottom() { return Hit(); }
protected func ContactTop() { return Hit(); }
protected func ContactLeft() { return Hit(); }
protected func ContactRight() { return Hit(); }

protected func Hit()
{
	//Message("I have hit something");
	if(GetEffect("Flight",this)) DoFireworks();
	else Sound("WoodHit");
}

protected func HitObject()
{
	DoFireworks();
}

func Launch(int angle)
{
	SetProperty("Collectible",0);
	SetCategory(C4D_Vehicle);
	SetAction("Fly");
	SetComDir(COMD_None);
	
	Exit();
	AddEffect("Flight",this,150,1,this);
	//AddEffect("HitCheck", this, 1,1, nil,nil, 0, 0);
	
	SetR(angle);
}

func DoFireworks(int speed)
{
	RemoveEffect("Flight",this);
	Fireworks();
	Explode(40);
}

func SetFuel(int new)
{
	fuel = new;
}

func GetFuel()
{
	return fuel;
}

local ActMap = {

Fly = {
	Prototype = Action,
	Name = "Fly",
	Procedure = DFA_FLOAT,
	Length = 1,
	Delay = 0,
	Wdt = 15,
	Hgt = 27
},
};
local PerspectiveR = 20000;
local PerspectiveTheta = 25;
local PerspectivePhi = 30;
local Name = "$Name$";
local Collectible = 1;
