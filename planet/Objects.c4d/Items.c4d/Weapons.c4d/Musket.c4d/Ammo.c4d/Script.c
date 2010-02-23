/*-- Musket Ball --*/

#include L_ST

public func MaxStackCount() { return 8; }

public func IsMusketAmmo() { return 1; }

public func ProjectileDamage() { return 10; }

protected func Hit()
{
	if(GetEffect("HitCheck",this))
	{
		RemoveEffect("HitCheck",this);
	
		Sound("BulletHitGround*.ogg");
		
		// TODO: ricochets? nice effects?
		RemoveObject();
	}
}

public func Launch(object shooter, int angle, int dist, int speed)
{
	AddEffect("HitCheck", this, 1,1, nil,nil, shooter);

	LaunchProjectile(angle+RandomX(-2, 2), dist, speed);	
	//Smush vertexes into one point
	SquishVertices(true);
	
	// sound
	Sound("BulletShot*.ogg");
}

private func HitObject(object pVictim)
{
	Sound("ProjectileHitLiving*.ogg");

	pVictim->DoEnergy(-ProjectileDamage());
	RemoveObject();
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

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}