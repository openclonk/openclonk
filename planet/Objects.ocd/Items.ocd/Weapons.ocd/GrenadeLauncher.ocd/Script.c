/**
	Grenade Launcher
	A single shot grenade launcher which fires dangerous iron bombs.
	
	@author: Clonkonaut
*/

#include Library_HasExtraSlot
#include Library_RangedWeapon

// Default timing values for animation set
// (Adjusted for speeed multiplier and stored in animation set by Library_RangedWeapon)
local DefaultLoadTime = 80;
local DefaultShootTime = 20;

local is_aiming;
local animation_set;
local loaded;
local reload;

local yOffset;

local holding;

local MuzzleUp; local MuzzleFront; local MuzzleDown; local MuzzleOffset;

/*-- Engine Callbacks --*/

func Initialize(...)
{
	//Tweaking options
	MuzzleUp = 12;
	MuzzleFront = 13;
	MuzzleDown = 16;
	MuzzleOffset = -8;
	
	loaded = false;
	is_aiming = false;
	
	animation_set = {
		AimMode        = AIM_Position, // The aiming animation is done by adjusting the animation position to fit the angle
		AnimationAim   = "MusketAimArms",
		AnimationLoad  = "MusketLoadArms",
		AnimationShoot = nil,
		WalkSpeed      = 84,
		WalkBack       = 56,
	};
	
	return _inherited(...);
}

func Hit()
{
	Sound("Hits::GeneralHit?");
}

func RejectCollect(id shotid, object shot)
{
	// Only collect grenade launcher ammo
	if(!(shot->~IsGrenadeLauncherAmmo())) return true;
}

/*-- Callbacks --*/

public func GetAnimationSet() { return animation_set; }

// Callback from the clonk when loading is finished
public func FinishedLoading(object clonk)
{
	SetLoaded();
	if(holding) clonk->StartAim(this);
	return holding; // false means stop here and reset the clonk
}

// Callback from the clonk, when he actually has stopped aiming
public func FinishedAiming(object clonk, int angle)
{
	if(!loaded) return;
	
	// Fire
	if(Contents(0) && Contents(0)->~IsGrenadeLauncherAmmo())
		FireWeapon(clonk, angle);
	Trajectory->Remove(clonk);
	clonk->StartShoot(this);
	return true;
}

// Can only be stacked with same state: loaded vs. non-loaded.
public func CanBeStackedWith(object other)
{
	return this->IsLoaded() == other->~IsLoaded() && inherited(other, ...);
}

/*-- Usage --*/

public func HoldingEnabled() { return true; }

public func RejectUse(object clonk)
{
	return !clonk->HasHandAction(false, false, true);
}

public func ControlUseStart(object clonk, int x, int y)
{
	// nothing in extraslot?
	if(!Contents(0))
	{
		// put something inside
		var obj = FindObject(Find_Container(clonk), Find_Func("IsGrenadeLauncherAmmo"));
		if (obj)
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

	is_aiming = true;
	holding = true;
	
	// reload weapon if not loaded yet
	if(!loaded)
		clonk->StartLoad(this);
	else
		clonk->StartAim(this);

	ControlUseHolding(clonk, x, y);
	
	return true;
}

public func ControlUseHolding(object clonk, ix, iy)
{
	var angle = Angle(0,0,ix,iy-MuzzleOffset);
	angle = Normalize(angle,-180);

	clonk->SetAimPosition(angle);
	
	// Show and update trajectory preview only when loaded.
	if (loaded)
	{
		var shot_x = Sin(180 - angle, MuzzleFront);
		var shot_y = Cos(180 - angle, MuzzleUp) + MuzzleOffset;
		Trajectory->Create(clonk, GetX() + shot_x, GetY() + shot_y, Cos(angle - 90, shooting_strength), Sin(angle - 90, shooting_strength));
	}
	return true;
}

public func ControlUseCancel(object clonk, int x, int y)
{
	clonk->CancelAiming(this);
	Trajectory->Remove(clonk);
	return true;
}

public func ControlUseStop(object clonk, ix, iy)
{
	holding = false;
	clonk->StopAim();
	return true;
}

public func Reset(clonk)
{
	is_aiming = false;
}

func FireWeapon(object clonk, int angle)
{
	var shot = Contents(0)->~TakeObject() ?? Contents(0);

	var IX=Sin(180-angle,MuzzleFront);
	var IY=Cos(180-angle,MuzzleUp)+MuzzleOffset;

	shot->LaunchProjectile(angle, 0, shooting_strength, IX, IY);
	shot->~Fuse(true);
	shot->SetController(clonk->GetController());

	loaded = false;
	// Reset transformation to indicate empty grenade launcher.
	this.PictureTransformation = this.Prototype.PictureTransformation;

	Sound("Objects::Weapons::Blunderbuss::GunShoot?");

	// Muzzle Flash & gun smoke
	if(Abs(Normalize(angle,-180)) > 90)
		IY=Cos(180-angle,MuzzleDown)+MuzzleOffset;

	var x = Sin(angle, 20);
	var y = -Cos(angle, 20);
	CreateParticle("Smoke", IX, IY, PV_Random(x - 20, x + 20), PV_Random(y - 20, y + 20), PV_Random(40, 60), Particles_Smoke(), 20);
	
	clonk->CreateMuzzleFlash(IX, IY, angle, 40);	
	CreateParticle("Flash", 0, 0, 0, 0, 8, Particles_Flash());
}

public func SetLoaded()
{
	loaded = true;
	// Change picture to indicate being loaded.
	this.PictureTransformation = Trans_Mul(Trans_Translate(-3000, 3000, 4000),Trans_Rotate(-45,0,0,1),Trans_Rotate(130,0,1,0));
	return;
}

public func IsLoaded() { return loaded; }

/*-- Production --*/

public func IsWeapon() { return true; }
public func IsArmoryProduct() { return true; }

/*-- Display --*/

public func GetCarryMode(object clonk, bool idle, bool nohand)
{
	if (idle || nohand)
		return CARRY_Back;

	return CARRY_Blunderbuss;
}

public func GetCarrySpecial()
{
	if (is_aiming) return "pos_hand2";
}

public func GetCarryTransform(object clonk, bool idle, bool nohand, bool second_on_back)
{
	if (is_aiming)
		return Trans_Mul(Trans_Rotate(90,1,0,0), Trans_Rotate(-10,0,0,1));

	if (idle)
	{
		if (!second_on_back)
			return Trans_Mul(Trans_Translate(0, 3000), Trans_Rotate(180, 1));
		else
			return Trans_Mul(Trans_Translate(3000, 3000, -1500), Trans_Rotate(180, 1), Trans_Rotate(-30, 0, 1));
	}
	if (nohand)
		return Trans_Translate(0, -3000);

	return Trans_Mul(Trans_Rotate(90,1,0,0), Trans_Rotate(-10,0,0,1));
}


public func OnRelaunchCreation()
{
	CreateContents(IronBomb);
}

public func IsExplosive()
{
	return !!FindObject(Find_Container(this), Find_Func("IsExplosive"));
}

func Definition(def)
{
	def.PictureTransformation = Trans_Mul(Trans_Translate(-3000, 1000, 1500),Trans_Rotate(170,0,1,0),Trans_Rotate(30,0,0,1));
	return _inherited(def, ...);
}

/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;
local ForceFreeHands = true;
local Components = {Wood = 1, Metal = 3};
// Initial velocity of the bomb
local shooting_strength = 75;
local ExtraSlotFilter = "IsGrenadeLauncherAmmo"; // For editor-provided ammo list
