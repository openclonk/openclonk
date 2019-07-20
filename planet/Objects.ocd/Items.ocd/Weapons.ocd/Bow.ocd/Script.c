/**
	Bow
	The standard bow. This object is a standard projectile weapon with an extra slot.
	
	@author: Newton
*/

#include Library_HasExtraSlot
#include Library_RangedWeapon
#include Library_Flammable

local fAiming;
local iArrowMesh;
local animation_set;

/*-- Engine Callbacks --*/

// Default timing values for animation set
// (Adjusted for speeed multiplier and stored in animation set by Library_RangedWeapon)
local DefaultLoadTime = 30;
local DefaultLoadTime2 = 5*30/20;
local DefaultShootTime = 20;
local DefaultShootTime2 = nil;

func Initialize(...)
{
	animation_set = {
		AimMode        = AIM_Position, // The aiming animation is done by adjusting the animation position to fit the angle
		AnimationAim   = "BowAimArms",
		AnimationLoad  = "BowLoadArms",
		AnimationShoot = nil,
		TurnType       = 1,
		WalkSpeed      = 84,
		WalkBack       = 56,
		AnimationReplacements = [
			["Walk", "BowWalk"],
			["Walk_Position", 20],
			["Stand", "BowStand"],
			["Jump", "BowJump"],
			["KneelDown", "BowKneel"]
		],
	};
	return _inherited(...);
}

func Hit()
{
	Sound("Hits::Materials::Wood::WoodHit?");
}

func RejectCollect(id arrowid, object arrows)
{
	// arrows are not arrows? decline!
	if (!(arrows->~IsArrow())) return true;
}

/*-- Callbacks --*/

public func GetAnimationSet() { return animation_set; }

// Attach the arrow during the animation
public func DuringLoad(object clonk) { return AddArrow(clonk); }

// Callback from the clonk when loading is finished
public func FinishedLoading(object clonk)
{
	clonk->~StartAim(this);
	return true;
}

// Callback from the clonk, when he actually has stopped aiming
// Calls "OnLaunchArrow"
public func FinishedAiming(object clonk, int angle)
{
	clonk->DetachMesh(iArrowMesh);
	iArrowMesh = nil;

	// shoot
	if (Contents(0))
	{
		if (Contents(0)->~IsArrow())
		{
			var arrow = Contents(0)->TakeObject();
			arrow->Launch(angle, shooting_strength, clonk, this);
			Sound("Objects::Weapons::Bow::Shoot?");
		}
	}

	// Open the hand to let the string go and play the fire animation
	PlayAnimation("Fire", 6, Anim_Linear(0, 0, GetAnimationLength("Fire"), animation_set["ShootTime"], ANIM_Hold));
	clonk->PlayAnimation("Close1Hand", 11, Anim_Const(0), Anim_Const(1000));
	clonk->StartShoot(this);
	return true;
}

/*
func Selection()
{
	Sound("Objects::Weapons::Bow::Draw");
}

func Deselection()
{
	Sound("Objects::Weapons::Bow::PutAwayBow");
}
*/

/*-- Usage --*/

public func HoldingEnabled() { return true; }

public func RejectUse(object clonk)
{
	// if the clonk doesn't have an action where he can use it's hands do nothing
	return !clonk->HasHandAction(false, false, true);
}

public func ControlUseStart(object clonk, int x, int y)
{
	// check for ammo
	if (!Contents(0))
	{
		// reload
		var obj = FindObject(Find_Container(clonk), Find_Func("IsArrow"));
		if (obj)
		{
			obj->Enter(this);
		}
	}
	
	if (!Contents(0))
	{
		// + sound or message that he doesnt have arrows anymore
		clonk->CancelUse();
		return true;
	}
	
	// Start aiming
	fAiming = 1;
	
	PlayAnimation("Draw", 6, Anim_Linear(0, 0, GetAnimationLength("Draw"), animation_set["LoadTime"], ANIM_Hold));

	clonk->StartLoad(this);

	ControlUseHolding(clonk, x, y);
	
	return true;
}

// Update the angle on mouse movement
public func ControlUseHolding(object clonk, int x, int y)
{
	// Save new angle
	var angle = Angle(0, 0, x, y);
	angle = Normalize(angle,-180);

	if (angle >  160) angle =  160;
	if (angle < -160) angle = -160;

	clonk->SetAimPosition(angle);
	
	return true;
}

// Stopping says the clonk to stop with aiming (he will go on untill he has finished loading and aiming at the given angle)
public func ControlUseStop(object clonk, int x, int y)
{
	clonk->StopAim();
	return true;
}

public func ControlUseCancel(object clonk, int x, int y)
{
	clonk->CancelAiming(this);
	return true;
}

// Called during loading then the arrow is added to the animation
public func AddArrow(object clonk)
{
	Sound("Objects::Weapons::Bow::Load?");
	iArrowMesh = clonk->AttachMesh(HelpArrow, "pos_hand1", "main", nil);
}

public func Reset(clonk)
{
	fAiming = 0;

	clonk->DetachMesh(iArrowMesh);
	iArrowMesh = nil;
	
	clonk->StopAnimation(clonk->GetRootAnimation(11));
	StopAnimation(GetRootAnimation(6));
}

func ClonkAimLimit(object clonk, int angle)
{
	angle = Normalize(angle,-180);
	if (Abs(angle) > 160) return false;
	if (clonk->GetDir() == 1 && angle < 0) return false;
	if (clonk->GetDir() == 0 && angle > 0) return false;
	return true;
}

func FxIntWalkSlowStart(pTarget, effect, fTmp, iValue)
{
	if (iValue == nil || iValue == 0)
	{
		iValue = 420;
	}
	pTarget->PushActionSpeed("Walk", iValue, GetID());
}

func FxIntWalkSlowStop(pTarget, effect)
{
	pTarget->PopActionSpeed("Walk", GetID());
}

/*-- Production --*/

public func IsWeapon() { return true; }
public func IsArmoryProduct() { return true; }

/*-- Display --*/

public func GetCarryMode(object clonk, bool idle)
{
	if (idle)
		return CARRY_Back;

	return CARRY_HandBack;
}

public func GetCarrySpecial(clonk)
{
	if (fAiming) return "pos_hand2";
}


public func OnRelaunchCreation()
{
	CreateContents(Arrow);
}

func Definition(def)
{
	def.PictureTransformation = Trans_Mul(Trans_Translate(-4000,-2000, 4000),Trans_Rotate(180, 0, 1, 0),Trans_Rotate(-45, 0, 0, 1));
	return _inherited(def, ...);
}

/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;
local ForceFreeHands = true;
local Components = {Wood = 3};
local BlastIncinerate = 30;
local MaterialIncinerate = true;
// Initial velocity of the arrow
local shooting_strength = 100;

local ExtraSlotFilter = "IsArrow"; // For editor-provided ammo list
