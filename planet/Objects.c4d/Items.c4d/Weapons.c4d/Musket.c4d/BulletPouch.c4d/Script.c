/*-- Shot Pouch --*/

#strict 2

local iAmmo;

protected func Initialize()
{
	iAmmo=8;
}

public func TakeAmmo()
{
	if(iAmmo!=0) {
	iAmmo=--iAmmo;
	}
}

public func CheckEmpty()
{
	if(AmmoCount()==0) RemoveObject();
}

public func AmmoCount() { return iAmmo; }

public func IsToolProduct() { return 1; }

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}