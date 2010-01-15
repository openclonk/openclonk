/*-- Projectile Weapon Base --*/

#strict 2

public func ProjectileType() { return MBLL; } 		//Object which is used as ammo
public func MuzzleVelocity() { return 300; }		//Speed of projectile on launch.
public func BarrelLength() { return 15; }			//Exit-point of projectiles... ie:End of a gun barrel
public func MagazineSize() { return 1; } 			//Amount of ammo this gun can hold
public func Accuracy() { return 3; }				//Spread of fired shots 0=Perfect Accuracy; 10=Horrible Accuracy;
public func MuzzleFlashID() { return FLSH; }		//ID tag of the muzzle effect. Return 0 for no muzzle effect. It should be noted different barrel lengths should have different muzzle flash objects (for proper offset).
public func FiringSound() { return "Blast3"; }		//Sound file to be played when weapon is fired.
public func WeaponReloadAction() { return "Walk"; }	//Animation for reloading weapon.
public func WeaponReloadSound() { return 0; }		//Sound file to be played when weapon is reloaded.

public func HoldingEnabled() { return true; }
public func HasExtraSlot() { return true; }

protected func ControlUse(object pClonk, ix, iy)
{
	return 1;
}

public func ControlUseHolding(object pClonk, ix, iy)
{
	//Angle Finder
	var IX=Sin(180-Angle(0,0,ix,iy),BarrelLength());
	var IY=Cos(180-Angle(0,0,ix,iy),BarrelLength());
	//Create debug dot to show muzzle-point
	CastParticles("DebugReticle",1,0,IX,IY,30,30,RGB(255,0,0),RGB(255,255,0));
}

protected func ControlUseStop(object pClonk, ix, iy)
{
	// Reload if empty
	if(!FindContents(ProjectileType()) && CheckCanUse(pClonk)==1)
	{
		while(FindObject(Find_Container(pClonk), Find_ID(ProjectileType())))
		{
		if(pClonk->GetAction()!= WeaponReloadAction()) pClonk->SetAction(WeaponReloadAction()); //Clonk's reloading animation
		//pClonk->FindContents(ProjectileType())->Enter(this());

		var ammo=pClonk->FindContents(ProjectileType());
		ammo->TakeObject()->Enter(this());
		Sound(WeaponReloadSound());
		Message("Click!", pClonk); //Remove all these messages when sound is working
		return 1; 
		}
	}

	// Fire
	if(FindContents(ProjectileType())) {
	FireWeapon(pClonk, ix, iy, BarrelLength(),13);
	return 1;
	}
return 1;
}

public func CheckCanUse(object pClonk)
{
	if(pClonk->GetOCF() & OCF_NotContained) return 1;
}

private func FireWeapon(object pClonk, int iX, int iY)
{
	FindContents(ProjectileType())->LaunchProjectile(Angle(0,0,iX,iY)+RandomX(-(Accuracy()), Accuracy()), BarrelLength(), MuzzleVelocity());
	Sound(FiringSound());
	Message("Bang!", pClonk); //For debug.

	//Muzzle Flash
	if(MuzzleFlashID()!=0) {
	var flash = pClonk->CreateObject(MuzzleFlashID());
	flash->SetAction("Flash",pClonk);
	flash->SetR(Angle(0,0,iX,iY)); 
	}
}

public func IsToolProduct() { return 1; }

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}