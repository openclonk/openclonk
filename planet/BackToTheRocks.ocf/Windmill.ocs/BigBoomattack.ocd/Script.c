/*--
	Boom attack
	Authors: Randrian, Newton

	An evil rocket which is hungry on the destruction of windmills
--*/

#include Boomattack

local hits;

protected func Construction()
{
	//flight length
	fuel=2000;
	hits = 0;
	SetGraphics(nil,Boomattack);
	SetObjDrawTransform(3500,0,0,0,3500,-40000);
}


protected func FxFlightTimer(object pTarget, effect, int iEffectTime)
{
	if(fuel<=0)
	{
		DoFireworks();
	}

	var ignition = iEffectTime % 15;
	
	if(!ignition)
	{
		SetXDir(Sin(GetR(),50), 100);
		SetYDir(-Cos(GetR(),50), 100);
	}
	
	
	if(!Random(iEffectTime % 15))
	{
		var sizemod = ignition*ignition/5;
		
		var x = -Sin(GetR(),70);
		var y = +Cos(GetR(),70);
		
		CreateParticle("ExploSmoke",x,y,RandomX(-1,1),RandomX(-1,1),RandomX(250,600),RGBa(130,130,130,100));
		CreateParticle("Thrust",x,y,GetXDir()/2,GetYDir()/2,RandomX(160,240)+sizemod,RGBa(255,200,200,200));
	}
	
	if(GetAction() != "Fly")
	{
		SetAction("Fly");
		SetComDir(COMD_None);
	}
		
	fuel--;
}

public func OnProjectileHit(object shot)
{
	shot->CreateObject(Rock,0,0)->Explode(10);
	
	if(hits > 10 * GetPlayerCount())
	{
		var gol = FindObject(Find_ID(Goal_SaveTheWindmills));
		DoFireworks();
	}
		
	++hits;
	var gol = FindObject(Find_ID(Goal_SaveTheWindmills));
	if(gol)	gol->IncShotScore(shot->GetController());
}

/* Contact */

protected func ContactBottom() { DoDrill(180); }
protected func ContactTop() { DoDrill(0); }
protected func ContactLeft() { DoDrill(-40); }
protected func ContactRight() { DoDrill(40); }

protected func Hit()
{
	DoDrill(0);
}

func DoDrill(int angle)
{
	var x = Sin(angle+GetR(), 40);
	var y = -Cos(angle+GetR(), 40);
	CreateObject(Rock,x,y,GetOwner())->Explode(30);
	SetXDir(Sin(GetR(),50), 100);
	SetYDir(-Cos(GetR(),50), 100);
}

func DoFireworks()
{
	RemoveEffect("Flight",this);
	for(var i = 0; i < 6; ++i)
		Fireworks();

	Explode(50);
}

local ActMap =  {

	Fly = {
		Prototype = Action,
		Name = "Fly",
		Procedure = DFA_FLOAT,
		Length = 1,
		Delay = 0,
		Wdt = 60,
		Hgt = 100
	},
};
