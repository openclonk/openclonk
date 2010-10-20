/*--
	Guided Rocket
	Author: Maikel
	
	Ammunition for the airplane
--*/

#include Rocket

public func MaxStackCount() { return 2; }

protected func FxIntFlightTimer(object target, int num, int time)
{
	if(fuel<=0)
		DoFireworks();

	var ignition = time % 9;
	
	// speed up rocket to certain speed.
	var angle = GetR();
	var speed = Distance(0, 0, GetXDir(100), GetYDir(100));
	speed += Max(0, (Rocket_MaxSpeed * 10 - speed) / 12);
	SetXDir(Sin(angle, speed), 100);
	SetYDir(-Cos(angle, speed), 100);
	

	// Check if there is a plane to explode at.
	if (time > 10)
		if (FindObject(Find_ID(Plane), Find_Exclude(shooter), Find_Exclude(this), Find_AtPoint()))
			DoFireworks();
			
	// Follow nearest other vehicle.
	for (var to_follow in FindObjects(Find_ID(Plane), Find_Exclude(shooter), Find_Exclude(this), Find_Distance(300), Sort_Distance()))
	{
		// Vehicle must be in 60 degree front wedge.
		var angle = Angle(GetX(), GetY(), to_follow->GetX(), to_follow->GetY());
		if (Inside(Abs(angle - GetR()), 3, 60))
		{
			if (angle > GetR())
				SetR(GetR() + 4);
			if (angle < GetR())
				SetR(GetR() - 4);
			break;				
		}
	}	
	
	var sizemod = ignition*ignition/5;
	
	var x = -Sin(GetR(),16);
	var y = Cos(GetR(),16);
	
	CreateParticle("ExploSmoke",x,y,RandomX(-1,1),RandomX(-1,2),RandomX(80,180),RGBa(130,130,130,75));
	CreateParticle("Thrust",x,y,GetXDir()/2,GetYDir()/2,RandomX(45,70)+sizemod,RGBa(255,200,200,160));
	
	fuel--;
}

func Definition(def) 
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(30,0,0,1),Trans_Rotate(-30,1,0,0),Trans_Scale(1300)),def);
}
local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
