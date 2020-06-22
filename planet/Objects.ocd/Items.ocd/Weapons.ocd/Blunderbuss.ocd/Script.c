/**
	Blunderbuss
	Shoots up to five bullets at once, ideal short range weapon.
	
	@author: Ringwaul
*/

#include Library_HasExtraSlot
#include Library_RangedWeapon

local is_aiming;
local animation_set;
local loaded;
local holding;
local musk_up, musk_front, musk_down, musk_offset;

// Default timing values for animation set
// (Adjusted for speeed multiplier and stored in animation set by Library_RangedWeapon)
local DefaultLoadTime = 80;
local DefaultShootTime = 20;


/*-- Engine Callbacks --*/

public func Initialize(...)
{
	// Tweaking options
	musk_up = 12;
	musk_front = 13;
	musk_down = 16;
	musk_offset = -8;
	
	loaded = false;
	is_aiming = false;
	
	animation_set = {
		AimMode        = AIM_Position, // The aiming animation is done by adjusting the animation position to fit the angle.
		AnimationAim   = "MusketAimArms",
		AnimationLoad  = "MusketLoadArms",
		AnimationShoot = nil,
		WalkSpeed      = 84,
		WalkBack       = 56,
	};
	
	return _inherited(...);
}

public func Hit()
{
	Sound("Hits::GeneralHit?");
}

public func RejectCollect(id shotid, object shot)
{
	// Only collect blunderbuss-ammo, which are bullets.
	if (shot->~IsBullet()) 
		return false;
	return true;
}


/*-- Callbacks --*/

public func GetAnimationSet() { return animation_set; }

// Callback from the clonk when loading is finished.
public func FinishedLoading(object clonk)
{
	SetLoaded();
	if (holding)
		clonk->StartAim(this);
	// False means stop here and reset the clonk.
	return holding;
}

// Callback from the clonk, when he actually has stopped aiming.
public func FinishedAiming(object clonk, int angle)
{
	if (!loaded) return;
	
	// Fire
	if (Contents(0) && Contents(0)->IsBullet())
		FireWeapon(clonk, angle);
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
	// Nothing in extraslot?
	if (!Contents(0))
	{
		// Put some bullets into the blunderbuss.
		var obj = FindObject(Find_Container(clonk), Find_Func("IsBullet"));
		if (obj)
			obj->Enter(this);
	}
	
	// Something in extraslot?
	if (!Contents(0))
	{
		clonk->CancelUse();
		Sound("Objects::Weapons::Blunderbuss::Click*");
		return true;
	}

	is_aiming = true;
	holding = true;
	
	// Reload weapon if not loaded yet.
	if (!loaded)
		clonk->StartLoad(this);
	else
		clonk->StartAim(this);

	ControlUseHolding(clonk, x, y);
	return true;
}

public func ControlUseHolding(object clonk, int x, int y)
{
	var angle = Angle(0, 0, x, y - musk_offset);
	angle = Normalize(angle, -180);
	clonk->SetAimPosition(angle);
	return true;
}

public func ControlUseCancel(object clonk, int x, int y)
{
	clonk->CancelAiming(this);
	return true;
}

public func ControlUseStop(object clonk, int x, int y)
{
	holding = false;
	clonk->StopAim();
	return true;
}

public func Reset(clonk)
{
	is_aiming = false;
}

public func FireWeapon(object clonk, int angle)
{
	// Calculate offset for shot and effects.
	var off_x = Sin(180 - angle, musk_front);
	var off_y = Cos(180 - angle, musk_up) + musk_offset;
	if (Abs(Normalize(angle, -180)) > 90)
		off_y = Cos(180 - angle, musk_down) + musk_offset;
	
	// Shoot up to five bullets at the same time.
	for (var cnt = 0; cnt < this.BulletsPerShot; cnt++)
	{
		if (!Contents(0))
			break;
		var shot = Contents(0)->TakeObject();
		shot->Launch(clonk, angle * 100 + RandomX(-this.BulletSpread, this.BulletSpread), RandomX(-1, 1), RandomX(this.BulletSpeed[0], this.BulletSpeed[1]), off_x, off_y, 100);
	}
	
	// Muzzle Flash & gun smoke.
	var x = Sin(angle, 20);
	var y = -Cos(angle, 20);
	CreateParticle("Smoke", off_x, off_y, PV_Random(x - 20, x + 20), PV_Random(y - 20, y + 20), PV_Random(40, 60), Particles_Smoke(), 20);
	clonk->CreateMuzzleFlash(off_x, off_y, angle, 20);
	CreateParticle("Flash", 0, 0, 0, 0, 8, Particles_Flash());
	
	// Change picture to indicate being unloaded.
	loaded = false;
	this.PictureTransformation = Trans_Mul(Trans_Translate(1500, 0, -1500), Trans_Rotate(170, 0, 1, 0), Trans_Rotate(30, 0, 0, 1));
	
	// Gun blast sound.
	Sound("Objects::Weapons::Blunderbuss::GunShoot?");
	return;
}

public func SetLoaded()
{
	loaded = true;
	// Change picture to indicate being loaded.
	this.PictureTransformation = Trans_Mul(Trans_Translate(500, 1000, 0), Trans_Rotate(130, 0, 1, 0), Trans_Rotate(20, 0, 0, 1));
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

public func GetCarrySpecial(clonk)
{
	if (is_aiming) return "pos_hand2";
}

public func GetCarryTransform()
{
	return Trans_Rotate(90, 1, 0, 0);
}

public func OnRelaunchCreation()
{
	CreateContents(LeadBullet);
}

public func Definition(def) 
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(1500, 0, -1500), Trans_Rotate(170, 0, 1, 0), Trans_Rotate(30, 0, 0, 1)), def);
	return _inherited(def, ...);
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;
local ForceFreeHands = true;
local Components = {Wood = 1, Metal = 2};

local BulletsPerShot = 5;
local BulletSpread = 300;
local BulletSpeed = [196, 204];

local ExtraSlotFilter = "IsBullet"; // For editor-provided ammo list
