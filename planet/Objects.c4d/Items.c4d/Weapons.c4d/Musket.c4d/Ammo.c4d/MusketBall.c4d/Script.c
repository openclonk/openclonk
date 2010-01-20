/*-- Musket Ball --*/

#strict 2
#include L_ST

public func MaxStackCount() { return 8; }

public func IsMusketAmmo() { return 1; }


protected func Hit()
{
	RemoveEffect("HitCheck",this);
	SetVelocity(Random(359)); //stops object for careening over the terrain, ricochets would be better :p
}

public func AffectShot(object shooter,int ix,int iy)
{
	AddEffect("HitCheck", this, 1,1, nil,nil, shooter);
}

private func HitObject(object pVictim)
{
	Message("Ouch!", pVictim); //Remove when sound works; for debug

	Punch(pVictim,RandomX(20,30));
	RemoveObject();
}

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}