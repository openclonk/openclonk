/*--
	Musket
	Author: Ringwaul

	A single shot musket which fires metal-shot at incredible speed.

--*/

#strict 2

//Uses the extra slot library
#include L_ES

local ReloadTimer;
local Loaded;

local yOffset;
local iBarrel;

protected func Initialize()
{
	//Tweaking options
	iBarrel=25;
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
	var IX=Sin(180-Angle(0,0,ix,iy),iBarrel);
	var IY=Cos(180-Angle(0,0,ix,iy),iBarrel);
	//Create debug dot to show muzzle-point
	CastParticles("DebugReticle",1,0,IX,IY,30,30,RGB(255,0,0),RGB(255,0,0));
	//DrawParticleLine("DebugReticle",0,0,IX,IY,2,20,RGB(30,30,30),RGB(100,100,100)); //Debug: Enable to see firing angle of musket
}

protected func ControlUseStop(object pClonk, ix, iy)
{
	// Reload if empty
	if(Contents(0)==nil && FindObject(Find_Container(pClonk), Find_Func("IsMusketAmmo"))==nil) return 1;
	if(Contents(0)==nil && Loaded==1) Loaded=0;
	if(!GetEffect("Reloading",this) && IsReloading()==true) ResumeReloading();
	if(CheckCanUse(pClonk)==true && IsReloading()==false && Loaded!=true)
	{
		if(Contents(0)==nil || Loaded==false)
		{
			ReloadWeapon(50);
			return 1;
		}
	}

	// Fire
	var IX=Sin(180-Angle(0,0,ix,iy),iBarrel);
	var IY=Cos(180-Angle(0,0,ix,iy),iBarrel);
	if(Contents(0)!=nil && PathFree(pClonk->GetX(),pClonk->GetY(),GetX()+IX,GetY()+IY)) //Stops musket from firing into ground or through walls.
	{
		if(Contents(0)->IsMusketAmmo()==1 && Loaded==true && CheckCanUse(pClonk)==true)
		{
		FireWeapon(pClonk, ix, iy);
		return 1;
		}
	}
return 1;
}

private func FireWeapon(object pClonk,iX,iY)
{
	var shot=Contents(0)->TakeObject();
	shot->LaunchProjectile(Angle(0,0,iX,iY)+RandomX(-2, 2), iBarrel, 300);
	var iAngle=Angle(0,0,iX,iY);
	shot->AffectShot(pClonk,iY,iX,iAngle);
	Loaded=false;

	Sound("Blast3");
	Message("Bang!", pClonk); //For debug.

	//Muzzle Flash
	/*var flash = pClonk->CreateObject(FLSH);
	flash->SetAction("Flash",pClonk);
	flash->SetR(Angle(0,0,iX,iY));
	*/

	//Muzzle Flash
	var iAngle=Angle(0,0,iX,iY);;
	var IX=Sin(180-Angle(0,0,iX,iY),iBarrel);
	var IY=Cos(180-Angle(0,0,iX,iY),iBarrel);
	CreateParticle("MuzzleFlash",IX,IY,+Sin(iAngle,500),-Cos(iAngle,500),300,RGB(255,255,255),pClonk);
	//Gun Smoke
	CastParticles("GunSmoke",10,3,IX,IY,20,50,RGBa(110,110,110,128),RGB(150,150,150,128));
	
}

private func ReloadWeapon(int iReloadTime)
{
	//Put ammo into gun's extra slot
	var Ammo;
	if(Ammo=FindObject(Find_Container(Contained()), Find_Func("IsMusketAmmo")))
	{
		if(Contents(0)==nil) Ammo->Enter(this()); //Only put new more ammo in gun if gun is empty
	}

	ReloadTimer=iReloadTime;
	AddEffect("Reloading",this,300,1,this,this);
}

protected func FxReloadingTimer(pTarget,iEffectNumber,iEffectTime)
{
	//If Clonk messes up reload, he must restart. :(
	if(Contained()==nil || CheckCanUse(Contained())==false)
	{
		PauseReloading();
		ReloadTimer=50;
	}

	if(ReloadTimer<=0) 
	{
		Loaded=true;
		return -1;
	}
	if(ReloadTimer>0) ReloadTimer=--ReloadTimer;
	Message("%d", Contained(), ReloadTimer);
}

protected func FxReloadingStop(pTarget,iEffectNumber,iReason,fTemp)
{
	Message("Pashunk!", Contained()); //This whole function is just for debug, unless there will be a sound for finishing a reload.
}

public func IsReloading()
{
	if(ReloadTimer>0) return true;
	return false;
}

public func PauseReloading()
{
	if(GetEffect("Reloading",this))
	{
		RemoveEffect("Reloading",this,0,1);
		return true;
	}
	return false;
}

public func ResumeReloading()
{
	if(ReloadTimer>0 && !GetEffect("Reloading",this))
	{
		AddEffect("Reloading",this,300,1,this,this);
		return true;
	}
	return false;
}

private func CheckCanUse(object pClonk)
{
	if(pClonk->GetOCF() & OCF_NotContained) 
	{
	if(pClonk->GetAction() == "Walk" || pClonk->GetAction() == "Jump") return true;
	}
	return false;
}

func RejectCollect(id shotid, object shot)
{
	//Only collect musket-balls
	if(!(shot->~IsMusketBall())) return true;
}

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}