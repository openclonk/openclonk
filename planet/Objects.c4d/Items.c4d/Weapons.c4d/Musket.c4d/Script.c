/*-- Musket --*/

#strict 2

local Ammo;
local IsLoaded;

public func BarrelLength() { return 15; }		//Exit-point of projectiles... ie:End of a gun barrel
public func MuzzleVelocity() { return 300; }		//Speed of projectile on launch
public func MagazineType() { return Contained()->FindContents(POCH); }		 //Collects bullets from this object... ie:Quiver
public func MagazineSize() { return 1; } 			//Amount of ammo this gun can hold
public func ProjectileType() { return MBLL; } 		//Object which fires from gun
public func Accuracy() { return 3; }		//Spread of fired shots 0=Perfect Accuracy; 10=Horrible Accuracy;

protected func Initialize() { Ammo=0; }

protected func ControlUse(object clonk, ix, iy)
{
	return 1;
}

protected func ControlUseStop(object pClonk, ix, iy)
{
	if(CheckCanUse(pClonk)==1) {

	// Reload from Empty
	if(MagazineType() && Ammo==0) {
	while(MagazineType()->AmmoCount()!=0 && Ammo<MagazineSize()) 
	{
	CreateContents(ProjectileType());
	MagazineType()->TakeAmmo();
	Ammo=++Ammo; 
	}
	MagazineType()->CheckEmpty();
	Message("Click!", pClonk); //Remove all these messages when sound is working
	return 1;
	}


	// Fire
	if(Ammo>=1) {
	FireWeapon(pClonk, ix, iy);
	Ammo=Ammo-1;
	if(Ammo==0) IsLoaded=0;
	return 1; }
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
	Sound("Blast3");
	Message("Bang!", pClonk);

	//Muzzle Flash
	pClonk->CreateContents(FLSH)->LaunchProjectile(Angle(0,0,iX,iY), BarrelLength(),0,0,13);
}

public func IsToolProduct() { return 1; }

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}