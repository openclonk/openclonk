/*--
	Boom attack
	Authors: Randrian (based on the boompack)

	An evil rocket which is hungry on the destruction of windmills
--*/

local fuel;

protected func Construction()
{
	//flight length
	fuel=1000;

	var iAngle = Angle(GetX(), GetY(), LandscapeWidth()/2, LandscapeHeight()/2);
	Launch(iAngle);
}


protected func FxFlightTimer(object pTarget, int iEffectNumber, int iEffectTime)
{
	if(fuel<=0)
	{
		DoFireworks();
	}

	var ignition = iEffectTime % 9;
	
	if(!ignition)
	{
//		var angle = GetR()+RandomX(-12,12);
//		SetXDir(3*GetXDir()/4+Sin(angle,50), 100);
//		SetYDir(3*GetYDir()/4-Cos(angle,50), 100);
//		SetR(angle);
	}
	
	var sizemod = ignition*ignition/3;
	
	var x = -Sin(GetR(),22);
	var y = +Cos(GetR(),22);
	
	CreateParticle("ExploSmoke",x,y,RandomX(-1,1),RandomX(-1,2),RandomX(120,280),RGBa(130,130,130,75));
    CreateParticle("Thrust",x,y,GetXDir()/2,GetYDir()/2,RandomX(80,120)+sizemod,RGBa(255,200,200,160));
	
	if(GetAction() != "Fly")
		SetAction("Fly");
}

public func IsProjectileTarget(target,shooter)
{
	return 1;
}

public func QueryCatchBlow(obj)
{
//	obj->Schedule("RemoveObject", 1);
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
	//Message("I have hit something",this);
	if(GetEffect("Flight",this)) DoFireworks();
	Sound("WoodHit");
}

func Launch(int angle)
{
	SetProperty("Collectible",0);
	SetCategory(C4D_Vehicle);
	SetAction("Fly");
	
	Exit();
	AddEffect("Flight",this,150,1,this,this);
	AddEffect("HitCheck", this, 1,1, nil,nil, 0, true);
	
	SetR(angle);
	SetVelocity(angle,5);
}

func DoFireworks(int speed)
{
	RemoveEffect("Flight",this);
	
	var color = HSL(Random(8)*32,255,127);
	
	if(!speed) speed = 12;
	for(var i=0; i<36; ++i)
	{
		var oangle = Random(70);
		var num = AddEffect("Firework", nil, 300, 1, nil, GetID(), Cos(oangle,speed), i*10+Random(5), GetX(), GetY());
		EffectVar(4,nil,num) = color;
	}
	
	for(var i=0; i<16; ++i)
	{
		CreateParticle("ExploSmoke",RandomX(-80,80),RandomX(-80,80),0,0,RandomX(500,700),RGBa(255,255,255,90));
	}
	CastParticles("Spark",60,190,0,0,40,70,color,color);
	
	CreateParticle("Flash",0,0,0,0,3500,color | (200 & 255)<<24);
	
	Explode(30);
}

func FxFireworkStart(object target, int num, int tmp, speed, angle, x, y, color)
{
	if(tmp) return;

	EffectVar(0, target, num) = speed*100;
	EffectVar(1, target, num) = angle;
	EffectVar(2, target, num) = x*100;
	EffectVar(3, target, num) = y*100;
}

func FxFireworkTimer(object target, int num, int time)
{
	var speed = EffectVar(0, target, num);
	var angle = EffectVar(1, target, num);
	var x = EffectVar(2, target, num);
	var y = EffectVar(3, target, num);
	
	if(time > 65) return -1;
	
	if(GBackSemiSolid(x/100,y/100)) return -1;
	
	// loose speed
	speed = 25*speed/26;
	
	var xdir = Sin(angle,speed);
	var ydir = -Cos(angle,speed);
	
	CreateParticle("Flash",x/100,y/100,xdir/100,ydir/100,50,EffectVar(4, target, num) | (200 & 255)<<24);
	
	// gravity
	ydir += GetGravity()*18/100;
	
	EffectVar(0, target, num) = speed;
	EffectVar(1, target, num) = Angle(0,0,xdir,ydir);
	EffectVar(2, target, num) = x+xdir;
	EffectVar(3, target, num) = y+ydir;
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
	Wdt=10,
	Hgt=18,
},
}, def);
  SetProperty("Name", "$Name$", def);
  SetProperty("Collectible",1, def);
  SetProperty("PerspectiveR", 20000, def);
  SetProperty("PerspectiveTheta", 25, def);
  SetProperty("PerspectivePhi", 30, def);
}
		  							