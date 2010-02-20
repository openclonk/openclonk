/*--
	Musket
	Author: Ringwaul

	A single shot musket which fires metal-shot at incredible speed.

--*/

//Uses the extra slot library
#include L_ES

local fAiming;
local iAim;
local fWait;

public func GetCarryMode(clonk) { if(fAiming >= 0) return CARRY_HandBack; }
public func GetCarrySpecial(clonk) { if(fAiming > 0) return "pos_hand2"; }
public func GetCarryBone()	{	return	"main";	}
/*public func GetCarryTransform()
{ 
	if(fAiming != 1) return Trans_Rotate(-90, 0, 1, 0);
}*/

func Definition(def) {
	SetProperty("Name", "$Name$", def);

	SetProperty("PerspectiveR", 12000, def);
}

local loaded;
local reload;

local yOffset;
local iBarrel;

local MuskUp; local MuskFront; local MuskDown; local MuskOffset;

protected func Initialize()
{
	//Tweaking options
	MuskUp = 12;
	MuskFront = 13;
	MuskDown = 16;
	MuskOffset = -8;
}

protected func HoldingEnabled() { return true; }

func ControlUseStart(object clonk, int x, int y)
{
	// if the clonk doesn't have an action where he can use it's hands do nothing
	if(!clonk->HasHandAction())
	{
		fWait = 1;
		return true;
	}
	fWait = 0;
	// nothing in extraslot?
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
		if(!loaded)
		{
			reload = 50;
			var iLoadTime = reload;
			iAim = clonk->PlayAnimation("MusketLoadArms", 10, Anim_Linear(0, 0, clonk->GetAnimationLength("MusketLoadArms"), iLoadTime, ANIM_Hold), Anim_Const(1000));
		}
		else
			iAim = clonk->PlayAnimation("MusketAimArms", 10, Anim_Const(0), Anim_Const(1000));
		fAiming = 1;
		clonk->SetHandAction(1);
		clonk->UpdateAttach();
		AddEffect("IntWalkSlow", clonk, 1, 0, 0, BOW1);
	}
	else clonk->CancelUse();
	
	return true;
}

func ControlUseHolding(object pClonk, ix, iy)
{
	if(fWait)
	{
		if(pClonk->HasHandAction())
			ControlUseStart(pClonk, ix, iy);
		return 0;
	}
	var p = pClonk->GetProcedure();
	if(p != "WALK" && p != "FLIGHT")
	{
		fAiming = 0;
		ResetClonk(pClonk);
		fWait = 1;
		return 1;
	}
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
		var angle = Angle(0,0,ix,iy-MuskOffset);
		//Angle Finder
		var IX=Sin(180-angle,MuskFront);
		var IY=Cos(180-angle,MuskUp)+MuskOffset;
		if(Abs(Normalize(angle,-180)) > 90)
			IY=Cos(180-angle,MuskDown)+MuskOffset;
		//Create debug dot to show muzzle-point
		//CastParticles("DebugReticle",1,0,IX,IY,30,30,RGB(255,0,0),RGB(255,0,0));
		//DrawParticleLine("DebugReticle",0,0,IX,IY,2,20,RGB(30,30,30),RGB(100,100,100)); //Debug: Enable to see firing angle of musket

		// angle
		angle = Normalize(angle,-180);

		var iTargetPosition = Abs(angle)*pClonk->GetAnimationLength("MusketAimArms")/180;
		var iPos = pClonk->GetAnimationPosition(iAim);
		iPos += BoundBy(iTargetPosition-iPos, -50, 50);
		pClonk->SetAnimationPosition(iAim, Anim_Const(iPos));
		if( (pClonk->GetComDir() == COMD_Stop && !pClonk->GetXDir()) || pClonk->GetAction() == "Jump")
		{
			if(pClonk->GetDir() == 1 && angle < 0) pClonk->SetDir(0);
			else if(pClonk->GetDir() == 0 && angle > 0) pClonk->SetDir(1);
		}
	}
	
	return true;
}

protected func ControlUseStop(object pClonk, ix, iy)
{
	if(fWait) return;
	if(!loaded)
	{
		ControlUseCancel(pClonk,ix,iy);
		return true;
	}
	
	if(!ClonkCanAim(pClonk)) return true;
	
	// Fire
	if(Contents(0))
	{
		if(Contents(0)->IsMusketAmmo() && PathFree(GetX(),GetY(),GetX()+Sin(180-Angle(0,0,ix,iy),iBarrel),GetY()+Cos(180-Angle(0,0,ix,iy),iBarrel)))
		{
			var angle = pClonk->GetAnimationPosition(iAim)*180/(pClonk->GetAnimationLength("MusketAimArms"));
			if(!pClonk->GetDir()) angle = 360-angle;
			FireWeapon(pClonk, ix, iy, angle);
			ScheduleCall(this, "ResetClonk", 20, 1, pClonk);
		}
	}
	return true;
}

protected func ControlUseCancel(object clonk, int x, int y)
{
	if(fWait) return;
  fAiming = 0;
  ResetClonk(clonk);
}

public func ResetClonk(clonk)
{
	fAiming = 0;

	clonk->SetHandAction(0);

  clonk->StopAnimation(clonk->GetRootAnimation(10));

	clonk->UpdateAttach();
	RemoveEffect("IntWalkSlow", clonk);
}

private func FireWeapon(object pClonk, iX, iY, iAngle)
{
	var shot = Contents(0)->TakeObject();
	var angle = Normalize(Angle(0,0,iX,iY)-180);
	if(angle>180 && pClonk->GetDir()==1) angle=angle-180;
	if(angle<180 && pClonk->GetDir()==0) angle=angle+180;
	shot->LaunchProjectile(angle+RandomX(-2, 2), iBarrel, 300);
	shot->AffectShot(pClonk,iY,iX,iAngle);
	
	loaded = false;

	Sound("Blast3");

	// Muzzle Flash
	var IX=Sin(180-iAngle,MuskFront);
	var IY=Cos(180-iAngle,MuskUp)+MuskOffset;
	if(Abs(Normalize(iAngle,-180)) > 90)
		IY=Cos(180-iAngle,MuskDown)+MuskOffset;
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
