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

local target_angle;

public func GetCarryMode(clonk) { if(fAiming >= 0) return CARRY_Musket; }
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
			reload = 100;
			var iLoadTime = reload;
			iAim = clonk->PlayAnimation("MusketLoadArms", 10, Anim_Linear(0, 0, clonk->GetAnimationLength("MusketLoadArms"), iLoadTime, ANIM_Hold), Anim_Const(1000));
		}
		else
			iAim = clonk->PlayAnimation("MusketAimArms", 10, Anim_Const(clonk->GetAnimationLength("MusketAimArms")/2), Anim_Const(1000));
		fAiming = 1;
		clonk->SetHandAction(1);
		clonk->UpdateAttach();
		AddEffect("IntWalkSlow", clonk, 1, 0, 0, BOW1);
		// Aim timer
		if(!GetEffect("IntAiming", clonk))
			AddEffect("IntAiming", clonk, 1, 1, this);
	}
	else clonk->CancelUse();
	
	return true;
}

func FxIntAimingTimer(clonk, number)
{
	if(fWait)
	{
		if(clonk->HasHandAction())
			ControlUseStart(clonk);
		return 0;
	}
	// check procedure
	if(!clonk->ReadyToAction())
	{
		ResetClonk(clonk);
		fWait = 1;
		return -1;
	}

	// loading...
	if(!loaded)
	{
		reload--;
		if(reload <= 0)
		{
			loaded = true;
			reload = 0;
			clonk->CancelUse();
			ResetClonk(clonk);
			return -1;
		}
	}
	// loaded
	else
	{
		//Angle Finder
		var IX=Sin(180-target_angle,MuskFront);
		var IY=Cos(180-target_angle,MuskUp)+MuskOffset;
		if(Abs(Normalize(target_angle,-180)) > 90)
			IY=Cos(180-target_angle,MuskDown)+MuskOffset;
		//Create debug dot to show muzzle-point
		//CastParticles("DebugReticle",1,0,IX,IY,30,30,RGB(255,0,0),RGB(255,0,0));
		//DrawParticleLine("DebugReticle",0,0,IX,IY,2,20,RGB(30,30,30),RGB(100,100,100)); //Debug: Enable to see firing angle of musket

		var iTargetPosition = Abs(target_angle)*clonk->GetAnimationLength("MusketAimArms")/180;
		var iPos = clonk->GetAnimationPosition(iAim);
		iPos += BoundBy(iTargetPosition-iPos, -50, 50);
		clonk->SetAnimationPosition(iAim, Anim_Const(iPos));
	}
}

func ControlUseHolding(object clonk, ix, iy)
{
	var angle = Angle(0,0,ix,iy-MuskOffset);
	target_angle = Normalize(angle,-180);
	
	return true;
}

protected func ControlUseStop(object clonk, ix, iy)
{
	if(!loaded) return;
	RemoveEffect("IntAiming", clonk);
	if(fWait) return;
	if(!loaded)
	{
		ControlUseCancel(clonk,ix,iy);
		return true;
	}
	
	// Fire
	if(Contents(0))
	{
		if(Contents(0)->IsMusketAmmo())
		{
//			if(PathFree(GetX(),GetY(),GetX()+Sin(180-Angle(0,0,ix,iy),iBarrel),GetY()+Cos(180-Angle(0,0,ix,iy),iBarrel)))
//			{
				var displayangle = clonk->GetAnimationPosition(iAim)*180/(clonk->GetAnimationLength("MusketAimArms"));
				if(!clonk->GetDir()) displayangle = 360-displayangle;
				
				FireWeapon(clonk, ix, iy, displayangle);
				ScheduleCall(this, "ResetClonk", 20, 1, clonk);
//			}
		}
	}
	return true;
}

protected func ControlUseCancel(object clonk, int x, int y)
{
	if(!loaded) return;
	RemoveEffect("IntAiming", clonk);
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

private func FireWeapon(object clonk, int iX, int iY, int displayangle)
{
	var shot = Contents(0)->TakeObject();
	var angle = Normalize(Angle(0,0,iX,iY)-180);
	if(angle>180 && clonk->GetDir()==1) angle=angle-180;
	if(angle<180 && clonk->GetDir()==0) angle=angle+180;
	shot->Launch(clonk,angle,iBarrel,300);
	
	loaded = false;

	Sound("GunShoot*.ogg");

	// Muzzle Flash & gun smoke
	var IX=Sin(180-displayangle,MuskFront);
	var IY=Cos(180-displayangle,MuskUp)+MuskOffset;
	if(Abs(Normalize(displayangle,-180)) > 90)
		IY=Cos(180-displayangle,MuskDown)+MuskOffset;

	for(var i=0; i<10; ++i)
	{
		var speed = RandomX(0,10);
		var r = displayangle;
		CreateParticle("ExploSmoke",IX,IY,+Sin(r,speed)+RandomX(-2,2),-Cos(r,speed)+RandomX(-2,2),RandomX(100,400),RGBa(255,255,255,50));
	}
	CreateParticle("MuzzleFlash",IX,IY,+Sin(displayangle,500),-Cos(displayangle,500),450,RGB(255,255,255),clonk);
	CreateParticle("Flash",0,0,0,0,800,RGBa(255,255,64,150));
}

func RejectCollect(id shotid, object shot)
{
	// Only collect musket-ammo
	if(!(shot->~IsMusketAmmo())) return true;
}
