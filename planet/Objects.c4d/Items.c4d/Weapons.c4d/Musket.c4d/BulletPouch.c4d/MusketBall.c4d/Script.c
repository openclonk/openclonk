/*-- Musket Ball --*/

#strict 2

public func IsAmmo() { return 1; }
public func BaseDamage() { return 20; }
public func MaxDamage() { return 30; }

protected func Hit()
{
	//Stops bullets from driving over the terrain. Should be removed as soon as ricochets are in physics.
	SetVelocity(Random(359), 5);
	Explode(5);
}

private func Check()
{
	var pVictim = FindObject(Find_OCF(OCF_Alive),Find_InRect(-15,-15,30,30),Find_NoContainer());
	// If bullet is flying, hurt victim
	if(GetOCF() & OCF_HitSpeed4 && GetOCF() & OCF_NotContained) { if(pVictim) BulletWound(pVictim); }
}

private func BulletWound(object pObj)
{
	Message("Hit!|%d", pObj, pObj->GetEnergy()); //Remove when sound works

	Sound("FleshHit*"); //Bullet-wound sound
	Punch(pObj,RandomX(BaseDamage(),(MaxDamage()-BaseDamage())));
//	pObj->CreateObject(MBLL); //shoddy ammo-saving script
	RemoveObject();
}

public func IsToolProduct() { return 1; }

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}