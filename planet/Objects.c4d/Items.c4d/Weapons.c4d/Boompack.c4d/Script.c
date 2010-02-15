/*--
	Boompack
	Author: Ringwaul

	A risky method of flight.
--*/

local fuel;
local rider;

static boomtrans;
static carrypos;

public func GetCarryMode(clonk) { return CARRY_BothHands; }
public func GetCarryTransform(clonk)	{	return Trans_Translate(-1500,2000,-3000);	} // TODO change when ck has fixed the bug
public func GetCarryPhase() { return 700; }

protected func Construction()
{
	//flight length
	fuel=100;
}

func ControlRight()
{	
	SetRDir(+3);
	return true;
}

func ControlLeft()
{
	SetRDir(-3);
	return true;
}

func ControlStop()
{
	SetRDir(0);
	return true;
}

func ControlUp(object clonk)
{
	JumpOff(clonk,20);
	return true;
}

func ControlUse(object clonk, ix, iy)
{	
	// already riding? Use ControlUse to jump off
	if(clonk->GetProcedure()=="ATTACH") return true;
	/*if(clonk->GetProcedure()=="ATTACH" && clonk->GetActionTarget() == this)
	{
		JumpOff(clonk,20);
		return true;
	}*/

	// only use during walk or jump
	if(clonk->GetProcedure()!="WALK" && clonk->GetProcedure()!="FLIGHT") return true;

	var angle=Angle(0,0,ix,iy);
	Launch(angle,clonk);

	return true;
}


protected func FxFlightTimer(object pTarget, int iEffectNumber, int iEffectTime)
{
	// clonk does sense the danger and with great presence of mind jumps of the rocket
	if(fuel<20 && rider)
	{
		JumpOff(rider,10);
	}

	if(fuel<=0)
	{
		DoFireworks();
	}

	var ignition = iEffectTime % 9;
	
	if(!ignition)
	{
		var angle = GetR()+RandomX(-12,12);
		SetXDir(3*GetXDir()/4+Sin(angle,24));
		SetYDir(3*GetYDir()/4-Cos(angle,24));
		SetR(angle);
	}
	
	var sizemod = ignition*ignition/3;
	
	CreateParticle("ExploSmoke",0,0,RandomX(-1,1),RandomX(-1,2),RandomX(120,280),RGBa(130,130,130,75));
    CreateParticle("Thrust",0,0,GetXDir()/2,GetYDir()/2,RandomX(80,120)+sizemod,RGBa(255,200,200,160));
	
	fuel--;
}

private func JumpOff(object clonk, int speed)
{
	if(!clonk) return;
	clonk->SetAction("Tumble");
	clonk->SetXDir(Sin(GetR(),speed)-10);
	clonk->SetYDir(Cos(GetR(),-speed));
	rider = nil;
}

protected func Hit()
{
	if(rider)
	{
		JumpOff(rider);
	}
	//Message("I have hit something",this);
	if(GetEffect("Flight",this)) DoFireworks();
	Sound("WoodHit");
}

func Launch(int angle, object clonk)
{
	SetProperty("Collectible",0);
	SetCategory(C4D_Vehicle);

	AddEffect("Flight",this,150,1,this,this);
	Exit();

	//Ride the rocket!
	if(clonk)
	{
		clonk->SetAction("Ride",this);
		rider=clonk;
	}

	var level = 16;
	var i=0, count = 3+level/8, r = Random(360);
	while(count > 0 && ++i < count*6) {
	  r += RandomX(40,80);
	  var smokex = +Sin(r,RandomX(level/4,level/2));
	  var smokey = -Cos(r,RandomX(level/4,level/2));
	  if(GBackSolid(smokex,smokey))
	    continue;
	  CreateSmokeTrail(2*level,r,smokex,smokey,nil,true);
	  count--;
	}
	
	SetR(angle);
	SetVelocity(angle,60);
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
		CreateParticle("ExploSmoke",RandomX(-80,80),RandomX(-80,80),0,0,RandomX(500,700),RGBa(255,255,255,80));
	}
	CastParticles("Spark",60,190,0,0,40,70,color,color);
	
	CreateParticle("FireworkGlow",0,0,0,0,3500,color | (200 & 255)<<24);
	
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
	
	CreateParticle("FireworkGlow",x/100,y/100,xdir/100,ydir/100,50,EffectVar(4, target, num) | (200 & 255)<<24);
	
	// gravity
	ydir += GetGravity()*18/100;
	
	EffectVar(0, target, num) = speed;
	EffectVar(1, target, num) = Angle(0,0,xdir,ydir);
	EffectVar(2, target, num) = x+xdir;
	EffectVar(3, target, num) = y+ydir;
}

func Definition(def) {
  SetProperty("Name", "$Name$", def);
  SetProperty("Collectible",1, def);
  SetProperty("PerspectiveR", 20000, def);
  SetProperty("PerspectiveTheta", 25, def);
  SetProperty("PerspectivePhi", 30, def);
}
		  							