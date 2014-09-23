// Golden barrel and infinite ammo for the cannon.

#appendto Cannon


public func Construction()
{
	_inherited(...);
	SetMeshMaterial("GoldenCannon", 0);
}

// Modify this function to not use the keg.
private func UseAnyStop(object clonk, int ix, int iy, int item)
{

	RemoveTrajectory(this);

	var projectile = clonk->GetHandItem(item);
	if (!projectile) // Needs a projectile
	{
		PlayerMessage(clonk->GetOwner(),"$TxtNeedsAmmo$");
		return true;
	}

	//Can't fire if cannon is cooling down or turning
	if(GetEffect("IntCooldown",this) || GetEffect("IntTurning",this)) return true;
	
	DoFire(projectile, clonk, Angle(0,0,ix,iy, angPrec));
	AddEffect("IntCooldown",this,1,1,this);

	return true;
}