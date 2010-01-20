/*-- Shot --*/

#strict 2
#include L_ST

public func MaxStackCount() { return 6; }
public func IsMusketAmmo() { return 1; }

local Shots;

protected func Initialize()
{
	Shots=5;
}

public func AffectShot(object shooter,int ix,int iy, int iAngle)
{
	//Creates 5 shots which spread etc.
	while(Shots>0)
	{
		var ShotBall=CreateObject(STBL);
		ShotBall->SetVelocity(iAngle+RandomX(-(5),5), RandomX(230,280));
		ShotBall->AffectShotBall(shooter);
		Shots=--Shots;
	}
	RemoveObject();
}

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}