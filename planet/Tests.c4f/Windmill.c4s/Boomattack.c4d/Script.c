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


protected func FxFlightTimer(object pTarget, int iEffectNumber, int iEffectTime)
{
	if(fuel<=0)
	{
		DoFireworks();
	}

	var ignition = iEffectTime % 10;
	
	if(!ignition)
	{
		var angle = GetR()+RandomX(-3,3);
		SetXDir(Sin(angle,100), 100);
		SetYDir(-Cos(angle,100), 100);
		SetR(angle);
	}
	
	
	if(!Random(iEffectTime % 5))
	{
		var sizemod = ignition*ignition/3;
		
		var x = -Sin(GetR(),22);
		var y = +Cos(GetR(),22);
		
		CreateParticle("ExploSmoke",x,y,RandomX(-1,1),RandomX(-1,2),RandomX(120,280),RGBa(130,130,130,75));
		CreateParticle("Thrust",x,y,GetXDir()/2,GetYDir()/2,RandomX(80,120)+sizemod,RGBa(255,200,200,160));
	}
	
	if(GetAction() != "Fly")
		SetAction("Fly");
		
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
	DoFireworks();
	var gol = FindObject(Find_ID(Goal_SaveTheWindmills));
	if(gol)	gol->IncShotScore(shot->GetOwner());
	return 1;
}

/* Contact */

protected func ContactBottom() { return Hit(); }
protected func ContactTop() { return Hit(); }
protected func ContactLeft() { return Hit(); }
protected func ContactRight() { return Hit(); }

protected func Hit()
{
	//Message("I have hit something",this);
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
	
	Exit();
	AddEffect("Flight",this,150,1,this,this);
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

func Definition(def) {
SetProperty("ActMap", {

Fly = {
	Prototype = Action,
	Name = "Fly",
	Procedure = DFA_FLOAT,
	Length = 1,
	Delay = 0,
	Wdt = 15,
	Hgt = 27
},
}, def);
  SetProperty("Name", "$Name$", def);
  SetProperty("Collectible",1, def);
  SetProperty("PerspectiveR", 20000, def);
  SetProperty("PerspectiveTheta", 25, def);
  SetProperty("PerspectivePhi", 30, def);
}
		  							