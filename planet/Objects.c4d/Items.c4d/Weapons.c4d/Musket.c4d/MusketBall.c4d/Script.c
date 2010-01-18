/*-- Musket Ball --*/

#strict 2
#include L_ST

public func MaxStackCount() { return 8; }

public func IsMusketAmmo() { return 1; }


protected func Hit()
{
	//Stops bullets from driving over the terrain. Should be removed as soon as ricochets are in physics.
	SetVelocity(Random(359), 5);
	RemoveEffect("HitCheck",this);
}

public func EffectShot(object shooter)
{
	AddEffect("HitCheck", this, 1,1, nil,nil, shooter);
}

private func HitObject(object pVictim)
{
	Message("Hit!|%d", pVictim, pVictim->GetEnergy()); //Remove when sound works

	Punch(pVictim,RandomX(20,30));
	RemoveObject();
}

public func IsToolProduct() { return 1; }

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}