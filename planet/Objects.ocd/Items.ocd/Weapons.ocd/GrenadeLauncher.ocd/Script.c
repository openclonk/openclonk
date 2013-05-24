/*--
	Grenade Launcher
	Author: Clonkonaut

	A single shot grenade launcher which fires dangerous iron bombs.

--*/

//Uses the extra slot library
#include Library_HasExtraSlot

func Hit()
{
	Sound("GeneralHit?");
}

local fAiming;

public func GetCarryMode(clonk) { if(fAiming >= 0) return CARRY_Musket; }
public func GetCarrySpecial(clonk) { if(fAiming > 0) return "pos_hand2"; }
public func GetCarryBone()	{	return	"main";	}
public func GetCarryTransform()
{
	return Trans_Mul(Trans_Rotate(-90,0,1,0), Trans_Rotate(10,1,0,0));
}

local animation_set;

func Initialize()
{
	//Tweaking options
	MuzzleUp = 12;
	MuzzleFront = 13;
	MuzzleDown = 16;
	MuzzleOffset = -8;
	
	animation_set = {
		AimMode        = AIM_Position, // The aiming animation is done by adjusting the animation position to fit the angle
		AnimationAim   = "MusketAimArms",
		AnimationLoad  = "MusketLoadArms",
		LoadTime       = 80,
		AnimationShoot = nil,
		ShootTime      = 20,
		WalkSpeed      = 84,
		WalkBack       = 56,
	};
}

public func GetAnimationSet() { return animation_set; }

local loaded;
local reload;

local yOffset;
local iBarrel;

local holding;

local MuzzleUp; local MuzzleFront; local MuzzleDown; local MuzzleOffset;

protected func HoldingEnabled() { return true; }

func ControlUseStart(object clonk, int x, int y)
{
	// if the clonk doesn't have an action where he can use it's hands do nothing
	if(!clonk->HasHandAction())
	{
		holding = true;
		return true;
	}

	// nothing in extraslot?
	if(!Contents(0))
	{
		// put something inside
		var obj;
		if(obj = FindObject(Find_Container(clonk), Find_Func("IsGrenadeLauncherAmmo")))
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

	holding = true;
	
	// reload weapon if not loaded yet
	if(!loaded)
		clonk->StartLoad(this);
	else
		clonk->StartAim(this);

	ControlUseHolding(clonk, x, y);
	
	return true;
}

// Callback from the clonk when loading is finished
public func FinishedLoading(object clonk)
{
	SetProperty("PictureTransformation",Trans_Mul(Trans_Translate(500,1000,-000),Trans_Rotate(130,0,1,0),Trans_Rotate(20,0,0,1)));
	loaded = true;
	if(holding) clonk->StartAim(this);
	return holding; // false means stop here and reset the clonk
}

func ControlUseHolding(object clonk, ix, iy)
{
	var angle = Angle(0,0,ix,iy-MuzzleOffset);
	angle = Normalize(angle,-180);

	clonk->SetAimPosition(angle);
	
	return true;
}

protected func ControlUseStop(object clonk, ix, iy)
{
	holding = false;
	clonk->StopAim();
	return -1;
}

// Callback from the clonk, when he actually has stopped aiming
public func FinishedAiming(object clonk, int angle)
{
	if(!loaded) return;
	
	// Fire
	if(Contents(0) && Contents(0)->IsGrenadeLauncherAmmo())
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
	var shot = Contents(0)->~TakeObject() ?? Contents(0);

	var IX=Sin(180-angle,MuzzleFront);
	var IY=Cos(180-angle,MuzzleUp)+MuzzleOffset;

	shot->LaunchProjectile(angle, 0, 75, IX, IY);
	shot->~Fuse(true);

	loaded = false;
	SetProperty("PictureTransformation",Trans_Mul(Trans_Translate(1500,0,-1500),Trans_Rotate(170,0,1,0),Trans_Rotate(30,0,0,1)));

	Sound("GunShoot?");

	// Muzzle Flash & gun smoke
	if(Abs(Normalize(angle,-180)) > 90)
		IY=Cos(180-angle,MuzzleDown)+MuzzleOffset;

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
	// Only collect grenade launcher ammo
	if(!(shot->~IsGrenadeLauncherAmmo())) return true;
}

public func IsWeapon() { return true; }
public func IsArmoryProduct() { return true; }



func Definition(def) {
	SetProperty("PictureTransformation",Trans_Mul(Trans_Translate(1500,0,-1500),Trans_Rotate(170,0,1,0),Trans_Rotate(30,0,0,1)),def);
}

local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local Collectible = 1;
local Rebuy = true;
