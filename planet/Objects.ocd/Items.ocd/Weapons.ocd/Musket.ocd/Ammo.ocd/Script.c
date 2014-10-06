/*-- Musket Ball --*/

#include Library_Stackable

public func MaxStackCount() { return 8; }

public func IsMusketAmmo() { return true; }

public func ProjectileDamage() { return 15; }
public func FlightTime() { return 30; }

protected func Hit()
{
	if(GetEffect("HitCheck",this))
	{
		RemoveEffect("HitCheck",this);
	
		Sound("BulletHitGround?");
		
		CreateParticle("StarSpark", 0, 0, PV_Random(-20, 20), PV_Random(-20, 20), PV_Random(10, 20), Particles_Glimmer(), 3);
		
		RemoveObject();
	}
}

public func Launch(object shooter, int angle, int dist, int speed, int offset_x, int offset_y)
{
	SetController(shooter->GetController());
	AddEffect("HitCheck", this, 1,1, nil,nil, shooter);

	LaunchProjectile(angle, dist, speed, offset_x, offset_y);
	
	// remove after some time
	SetAction("Travel");
	SetComDir(COMD_None);

	//Smush vertexes into one point
	SquishVertices(true);
	
	// neat trail
	CreateObject(BulletTrail,0,0)->Set(2, 200, this);
	
	// sound
	Sound("BulletShot?");
}

public func HitObject(object obj)
{
	ProjectileHit(obj,ProjectileDamage(),ProjectileHit_tumble);
	RemoveObject();
}

// called by successful hit of object after from ProjectileHit(...)
public func OnStrike(object obj)
{
	if(obj->GetAlive())
		Sound("ProjectileHitLiving?");
	else
		Sound("BulletHitGround?");
}

func UpdatePicture()
{
	var Shots=GetStackCount();
	if(Shots>=MaxStackCount()) SetGraphics(nil);
	if(Shots<MaxStackCount()) SetGraphics(Format("%d",Shots));
	//Realigns vertex points if >1
	if(Shots==1)
	{
		SquishVertices(true);
	}

	if(Shots>1)
	{
		SquishVertices(false);
	}
	_inherited(...);
}

private func SquishVertices(bool squish)
{
	if(squish==true)
	{
		SetVertex(1,VTX_X,0,2);
		SetVertex(1,VTX_Y,0,2);
		SetVertex(2,VTX_X,0,2);
		SetVertex(2,VTX_Y,0,2);
	return 1;
	}

	if(squish!=true)
	{
		SetVertex(1,VTX_X,-3,2);
		SetVertex(1,VTX_Y,1,2);
		SetVertex(2,VTX_X,3,2);
		SetVertex(2,VTX_Y,1,2);
	return 0;
	}

}

protected func Traveling()
{
	if(GetActTime() > FlightTime()) RemoveObject();
}

public func TrailColor(int time)
{
	return RGBa(255,255,255,240*Max(0,FlightTime()-time)/FlightTime());
}

public func IsArmoryProduct() { return true; }

local ActMap = {

	Travel = {
		Prototype = Action,
		Name = "Travel",
		Procedure = DFA_FLOAT,
		NextAction = "Travel",
		Length = 1,
		Delay = 1,
		FacetBase = 1,
		StartCall = "Traveling"
	},
};
local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
local Rebuy = true;