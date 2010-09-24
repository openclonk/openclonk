/*--
	Rocket
	Author: Maikel
	
	Ammunition for the airplane
--*/

#include Library_Stackable

static const Rocket_MaxSpeed = 150;

local fuel;
local shooter;

public func MaxStackCount() { return 3; }
public func IsPlaneWeapon() { return true; }
public func GetCoolDownTime() { return 20; }

protected func Initialize()
{
	//flight length
	fuel=100;
}

public func SetShooter(object plane)
{
	shooter = plane;
	return;
}

public func Launch(object plane, int angle)
{
	var rocket = TakeObject();
	rocket->SetProperty("Collectible",0);
	rocket->SetCategory(C4D_Vehicle);
	rocket->SetShooter(plane);

	rocket->Exit();
	AddEffect("IntFlight", rocket, 150, 1, rocket);

	var level = 6;
	var i=0, count = 3+level/8, r = Random(360);
	while (count > 0 && ++i < count*6)
	{
		r += RandomX(40,80);
		var smokex = +Sin(r,RandomX(level/4,level/2));
		var smokey = -Cos(r,RandomX(level/4,level/2));
		if(GBackSolid(smokex,smokey))
			continue;
		CreateSmokeTrail(2*level,r,smokex,smokey,nil,true);
		count--;
	}
	
	rocket->SetR(plane->GetR());
	var speed = Distance(0, 0, plane->GetXDir(), plane->GetYDir());
	rocket->SetVelocity(angle, speed);
}

protected func FxIntFlightTimer(object target, int num, int time)
{
	if(fuel<=0)
		DoFireworks();

	var ignition = time % 9;
	
	// speed up rocket to certain speed.
	var angle = GetR();
	var speed = Distance(0, 0, GetXDir(100), GetYDir(100));
	speed += Max(0, (Rocket_MaxSpeed * 10 - speed) / 15);
	SetXDir(Sin(angle, speed), 100);
	SetYDir(-Cos(angle, speed), 100);

	// Check if there is a plane to explode at.
	if (time > 10)
		if (FindObject(Find_ID(Plane), Find_Exclude(shooter), Find_Exclude(this), Find_AtPoint()))
			DoFireworks();
	
	var sizemod = ignition*ignition/5;
	
	var x = -Sin(GetR(),16);
	var y = Cos(GetR(),16);
	
	CreateParticle("ExploSmoke",x,y,RandomX(-1,1),RandomX(-1,2),RandomX(80,180),RGBa(130,130,130,75));
	CreateParticle("Thrust",x,y,GetXDir()/2,GetYDir()/2,RandomX(45,70)+sizemod,RGBa(255,200,200,160));
	
	fuel--;
}

protected func Hit()
{
	if(GetEffect("IntFlight",this)) DoFireworks();
	Sound("WoodHit");
}

func DoFireworks()
{
	RemoveEffect("IntFlight",this);
	Fireworks();
	Explode(30);
}

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(30,0,0,1),Trans_Rotate(-30,1,0,0),Trans_Scale(1300)),def);
}
local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
