/*--
	Musket
	Author: Ringwaul

	A single shot musket which fires metal-shot at incredible speed.

--*/

#strict 2
#include L_ES

local ReloadTimer;
local ReadyToFire;

protected func Initialize()
{
	ReloadTimer=100;
}

protected func HoldingEnabled() { return true; }

protected func ControlUse(object pClonk, ix, iy)
{
	//Don't throw the musket away!
	return 1;
}

public func ControlUseHolding(object pClonk, ix, iy)
{
	//Angle Finder
	var IX=Sin(180-Angle(0,0,ix,iy),25);
	var IY=Cos(180-Angle(0,0,ix,iy),25);
	//Create debug dot to show muzzle-point
	CastParticles("DebugReticle",1,0,IX,IY,30,30,RGB(255,0,0),RGB(255,0,0));

	
	// Reload if empty
	if(Contents(0)==nil && CheckCanUse(pClonk)==1)
	{
		ReloadWeapon(pClonk);
	}

}

protected func ControlUseStop(object pClonk, ix, iy)
{
	//Stops bow-like auto-firing
	if(ReadyToFire!=1 && ReloadTimer==100)
	{
		ReadyToFire=1;
		return 1;
	}

	// Fire
	var IX=Sin(180-Angle(0,0,ix,iy),25);
	var IY=Cos(180-Angle(0,0,ix,iy),25);
	if(Contents(0)!=nil && PathFree(pClonk->GetX(),pClonk->GetY(),GetX()+IX,GetY()+IY)) //Stops musket from firing into ground or through walls.
	{
		if(Contents(0)->IsMusketAmmo()==1)
		{
		FireWeapon(pClonk, ix, iy);
		return 1;
		}
	}
return 1;
}

public func CheckCanUse(object pClonk)
{
	if(pClonk->GetOCF() & OCF_NotContained)
	return 1;
}

private func ReloadWeapon(object pClonk)
{
	var Ammo;
	if(Ammo=FindObject(Find_Container(pClonk), Find_Func("IsMusketAmmo"))) 
	{
		if(ReloadTimer>0) 
		{
			ReloadTimer=ReloadTimer-1;
			Message("%d", Contained(), ReloadTimer);
			return 1;
		}

		if(ReloadTimer<=0)
		{
			Ammo->TakeObject()->Enter(this());
			Message("Click!", pClonk); //Remove all these messages when sound is working
			ReloadTimer=100;
			ReadyToFire=0;
			return 1;
		}
	}
}

private func FireWeapon(object pClonk, int iX, int iY)
{
	var shot=Contents(0);
	shot->EffectShot(pClonk);
	shot->LaunchProjectile(Angle(0,0,iX,iY)+RandomX(-3, 3), 25, 300);

	Sound("Blast3");
	Message("Bang!", pClonk); //For debug.

	//Muzzle Flash
	var flash = pClonk->CreateObject(FLSH);
	flash->SetAction("Flash",pClonk);
	flash->SetR(Angle(0,0,iX,iY));
	//puff smoke from barrel
	//Gun Smoke
	var IX=Sin(180-Angle(0,0,iX,iY),25);
	var IY=Cos(180-Angle(0,0,iX,iY),25);
	CastParticles("GunSmoke",10,3,IX,IY,20,50,RGBa(110,110,110,128),RGB(150,150,150,128));
	
}

func RejectCollect(id shotid, object shot)
{
	//Only collect musket-balls
	if(!(shot->~IsMusketBall())) return true;
}

public func IsToolProduct() { return 1; }

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}