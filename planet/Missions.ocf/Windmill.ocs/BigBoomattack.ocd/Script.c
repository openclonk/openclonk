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
		return;
	}
	
	if(GetAction() != "Fly")
	{
		SetAction("Fly");
		SetComDir(COMD_None);
	}
	
	var ignition = iEffectTime % 15;
	
	if(!ignition)
	{
		SetXDir(Sin(GetR(),50), 100);
		SetYDir(-Cos(GetR(),50), 100);
	}
	
	
	var x = -Sin(GetR(), 80);
	var y = +Cos(GetR(), 80);

	var xdir = GetXDir() / 2;
	var ydir = GetYDir() / 2;
	CreateParticle("FireDense", x, y, PV_Random(xdir - 20, xdir + 20), PV_Random(ydir - 20, ydir + 20), PV_Random(16 * 6, 38 * 6), {Prototype = Particles_Thrust(), Size = PV_Random(40, 60)}, 5);
			
	fuel--;
}

public func OnProjectileHit(object shot)
{
	shot->CreateObjectAbove(Rock,0,0)->Explode(10);
	
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
	CreateObjectAbove(Rock,x,y,GetOwner())->Explode(30);
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
