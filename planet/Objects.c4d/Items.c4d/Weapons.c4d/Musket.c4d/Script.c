/*--
	Musket
	Author: Ringwaul

	A single shot musket which fires metal-shot at incredible speed.

--*/

//Uses the extra slot library
#include L_ES

func Definition(def) {
	SetProperty("Name", "$Name$", def);
}

local loaded;
local reload;

local yOffset;
local iBarrel;

protected func Initialize()
{
	//Tweaking options
	iBarrel=25;
}

protected func HoldingEnabled() { return true; }

func ControlUseStart(object clonk, int x, int y)
{
	// nothign in extraslot?
	if(!Contents(0))
	{
		// put something inside
		var obj;
		if(obj = FindObject(Find_Container(clonk), Find_Func("IsMusketAmmo")))
		{
			obj->Enter(this);
		}
	}
	
	// something in extraslot
	if(Contents(0))
	{
		// reload weapon if not loaded yet
		if(!loaded) reload = 50;
	}
	else clonk->CancelUse();
	
	return true;
}

func ControlUseHolding(object pClonk, ix, iy)
{
	// loading...
	if(!loaded)
	{
		// If Clonk messes up during reload, he must restart. :(
		if(Contained()->GetProcedure() != "WALK")
			pClonk->CancelUse();
			
		// count
		Message("%d",pClonk,reload);
		
		reload--;
		if(reload <= 0)
		{
			loaded = true;
			reload = 0;
			Message("Pashunk!", Contained());
			pClonk->CancelUse();
		}
	}
	// loaded
	else
	{
		//Angle Finder
		var IX=Sin(180-Angle(0,0,ix,iy),iBarrel);
		var IY=Cos(180-Angle(0,0,ix,iy),iBarrel);
		//Create debug dot to show muzzle-point
		CastParticles("DebugReticle",1,0,IX,IY,30,30,RGB(255,0,0),RGB(255,0,0));
		//DrawParticleLine("DebugReticle",0,0,IX,IY,2,20,RGB(30,30,30),RGB(100,100,100)); //Debug: Enable to see firing angle of musket
	}
	
	return true;
}

protected func ControlUseStop(object pClonk, ix, iy)
{
	if(!loaded) return true;
	
	if(!ClonkCanAim(pClonk)) return true;
	
	// Fire
	if(Contents(0))
	{
		if(Contents(0)->IsMusketAmmo())
		{
			FireWeapon(pClonk, ix, iy);
		}
	}
	return true;
}

private func FireWeapon(object pClonk,iX,iY)
{
	var shot = Contents(0)->TakeObject();
	shot->LaunchProjectile(Angle(0,0,iX,iY)+RandomX(-2, 2), iBarrel, 300);
	var iAngle=Angle(0,0,iX,iY);
	shot->AffectShot(pClonk,iY,iX,iAngle);
	
	loaded = false;

	Sound("Blast3");
	
	Message("Bang!", pClonk); //For debug.

	// Muzzle Flash
	var iAngle=Angle(0,0,iX,iY);
	var IX=Sin(180-Angle(0,0,iX,iY),iBarrel);
	var IY=Cos(180-Angle(0,0,iX,iY),iBarrel);
	CreateParticle("MuzzleFlash",IX,IY,+Sin(iAngle,500),-Cos(iAngle,500),300,RGB(255,255,255),pClonk);
	//Gun Smoke
	CastParticles("GunSmoke",10,3,IX,IY,20,50,RGBa(110,110,110,128),RGB(150,150,150,128));
}

private func ClonkCanAim(object clonk)
{
	var p = clonk->GetProcedure();
	if(p != "WALK" && p != "ATTACH" && p != "FLIGHT") return false;
	return true;
}

func RejectCollect(id shotid, object shot)
{
	// Only collect musket-ammo
	if(!(shot->~IsMusketAmmo())) return true;
}
