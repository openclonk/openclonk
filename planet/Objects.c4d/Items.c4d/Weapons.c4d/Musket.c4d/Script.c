/*--
	Musket
	Author: Ringwaul

	A single shot musket which fires metal-shot at incredible speed.

--*/

//Uses the extra slot library
#include Library_HasExtraSlot

local fAiming;

public func GetCarryMode(clonk) { if(fAiming >= 0) return CARRY_Musket; }
public func GetCarrySpecial(clonk) { if(fAiming > 0) return "pos_hand2"; }
public func GetCarryBone()	{	return	"main";	}
/*public func GetCarryTransform()
{ 
	if(fAiming != 1) return Trans_Rotate(-90, 0, 1, 0);
}*/

local animation_set;

func Initialize()
{
	//Tweaking options
	MuskUp = 12;
	MuskFront = 13;
	MuskDown = 16;
	MuskOffset = -8;
	
	animation_set = {
		AimMode        = AIM_Position, // The aiming animation is done by adjusting the animation position to fit the angle
		AnimationAim   = "MusketAimArms",
		AnimationLoad  = "MusketLoadArms",
		LoadTime       = 100,
		AnimationShoot = nil,
		ShootTime      = 20,
		WalkSpeed      = 30000,
	};
}

public func GetAnimationSet() { return animation_set; }

local loaded;
local reload;

local yOffset;
local iBarrel;

local MuskUp; local MuskFront; local MuskDown; local MuskOffset;

protected func HoldingEnabled() { return true; }

func ControlUseStart(object clonk, int x, int y)
{
	// if the clonk doesn't have an action where he can use it's hands do nothing
	if(!clonk->HasHandAction())
		return true;

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
	if(!Contents(0))
	{
		clonk->CancelUse();
		return true;
	}

	fAiming = 1;

	// reload weapon if not loaded yet
	if(!loaded)
		clonk->StartLoad(this);
	else
		clonk->StartAim(this);

	return true;
}

// Callback from the clonk when loading is finished
public func FinishedLoading(object clonk)
{
	loaded = true;
	return false; // false means stop here and reset the clonk
}

func ControlUseHolding(object clonk, ix, iy)
{
	var angle = Angle(0,0,ix,iy-MuskOffset);
	angle = Normalize(angle,-180);

	clonk->SetAimPosition(angle);
	
	return true;
}

protected func ControlUseStop(object clonk, ix, iy)
{
	clonk->StopAim();
	return true;
}

// Callback from the clonk, when he actually has stopped aiming
public func FinishedAiming(object clonk, int angle)
{
	if(!loaded) return;
	
	// Fire
	if(Contents(0) && Contents(0)->IsMusketAmmo())
		FireWeapon(clonk, angle);
	clonk->StartShoot(this);
	return true;
}

protected func ControlUseCancel(object clonk, int x, int y)
{
	clonk->CancelAiming(this);
	return true;
}

public func Reset(clonk)
{
	fAiming = 0;
}

private func FireWeapon(object clonk, int angle)
{
	var shot = Contents(0)->TakeObject();
	shot->Launch(clonk,angle,iBarrel,300);
	
	loaded = false;

	Sound("GunShoot*.ogg");

	// Muzzle Flash & gun smoke
	var IX=Sin(180-angle,MuskFront);
	var IY=Cos(180-angle,MuskUp)+MuskOffset;
	if(Abs(Normalize(angle,-180)) > 90)
		IY=Cos(180-angle,MuskDown)+MuskOffset;

	for(var i=0; i<10; ++i)
	{
		var speed = RandomX(0,10);
		var r = angle;
		CreateParticle("ExploSmoke",IX,IY,+Sin(r,speed)+RandomX(-2,2),-Cos(r,speed)+RandomX(-2,2),RandomX(100,400),RGBa(255,255,255,50));
	}
	CreateParticle("MuzzleFlash",IX,IY,+Sin(angle,500),-Cos(angle,500),450,RGB(255,255,255),clonk);
	CreateParticle("Flash",0,0,0,0,800,RGBa(255,255,64,150));
}

func RejectCollect(id shotid, object shot)
{
	// Only collect musket-ammo
	if(!(shot->~IsMusketAmmo())) return true;
}

func Definition(def) {
	SetProperty("Name", "$Name$", def);
}
