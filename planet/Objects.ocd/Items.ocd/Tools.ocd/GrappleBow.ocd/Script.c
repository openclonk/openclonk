/*
	Grapple Bow
	Author: Randrian

	A crossbow which is enabled to fire grappling hooks, also has a winching system.
*/

func Hit()
{
	Sound("GeneralHit?");
}

local fAiming;

local hook;
local hook_attach;

public func GetCarrySpecial(clonk) { if(fAiming) return "pos_hand2"; }
public func GetCarryBone2(clonk) { return "main2"; }
public func GetCarryMode(clonk) { if(hook && hook->Contained() == nil) return CARRY_Back; if(fAiming >= 0) return CARRY_Grappler; }

/* +++++++++++ Controls ++++++++++++++ */

// holding callbacks are made
public func HoldingEnabled() { return true; }

local animation_set;

func Initialize()
{
	animation_set = {
		AimMode        = AIM_Position, // The aiming animation is done by adjusting the animation position to fit the angle
		AnimationAim   = "CrossbowAimArms",
		AnimationShoot = nil,
		ShootTime      = 20,
		TurnType       = 1,
		WalkSpeed      = 84,
		WalkBack       = 56,
		AimSpeed       = 20,            // the speed of aiming
	};
	OnRopeBreak();
}

public func SetHook(object new_hook)
{
	hook = new_hook;
}

private func EnsureHook()
{
	// Create hook if it went missing
	if(!hook) hook = CreateObject(GrappleHook, 0, 0, NO_OWNER);
	return hook;
}

public func OnRopeBreak()
{
	if(hook_attach)
		DetachMesh(hook_attach);

	EnsureHook();
	hook->Enter(this);
	hook_attach = AttachMesh(hook, "bolt", "main");
	PlayAnimation("Load", 5, Anim_Const(GetAnimationLength("Load")), Anim_Const(1000));
}

public func DrawRopeIn()
{
	if (hook)
	{
		var rope = hook->GetRope();
		if (rope)
			rope->DrawIn();
	}
}

protected func Destruction()
{
	if (hook)
	{
		var rope = hook->GetRope();
		if (rope)
			rope->BreakRope();
	}
}

protected func Departure()
{
	if (hook)
	{
		var rope = hook->GetRope();
		if (rope)
			rope->DrawIn();
	}
}

public func GetAnimationSet() { return animation_set; }

public func ControlUseStart(object clonk, int x, int y)
{
	// Burned?
	if (GetCon()<100) return false;
	// Cut rope, or otherwise remove helper object.
	EnsureHook();
	if (hook->Contained() != this)
	{
		var rope = hook->GetRope();
		if (rope)
		{
			rope->DrawIn();
		//	rope->BreakRope();
			return true;
		}
		else
		{
			hook->Enter(this);
		}
	}

	// if the clonk doesn't have an action where he can use it's hands do nothing
	if(!clonk->HasHandAction())
	{
		return true;
	}

	// Start aiming
	fAiming = 1;

	ControlUseHolding(clonk, x, y);

	FinishedLoading(clonk);

	return true;
}

// Callback from the clonk when loading is finished
public func FinishedLoading(object clonk)
{
	clonk->~StartAim(this);
	return true;
}

// Update the angle on mouse movement
public func ControlUseHolding(object clonk, int x, int y)
{
	// Save new angle
	var angle = Angle(0,0,x,y);
	angle = Normalize(angle,-180);

	if(angle >  160) angle =  160;
	if(angle < -160) angle = -160;

	clonk->SetAimPosition(angle);

	return true;
}

// Stopping says the clonk to stop with aiming (he will go on untill he has finished loading and aiming at the given angle)
public func ControlUseStop(object clonk, int x, int y)
{
	clonk->StopAim();
	return true;
}

// Callback from the clonk, when he actually has stopped aiming
public func FinishedAiming(object clonk, int angle)
{
	if (GetCon()<100) return false;
	EnsureHook();
	DetachMesh(hook_attach);
	hook_attach = nil;

	hook->Exit();
	hook->Launch(angle, 100, clonk, this);
	DetachMesh(hook_attach);
	Sound("BowShoot?");

	// Open the hand to let the string go and play the fire animation
	PlayAnimation("Fire", 6, Anim_Linear(0, 0, GetAnimationLength("Fire"), animation_set["ShootTime"], ANIM_Hold), Anim_Const(1000));
	clonk->StartShoot(this);
	return true;
}

public func ControlUseCancel(object clonk, int x, int y)
{
	clonk->CancelAiming();
	return true;
}

public func OnPauseAim(object clonk)
{
	Reset(clonk);
}

public func OnRestartAim(object clonk)
{
	ControlUseStart(clonk);
	if(fAiming) return true;
	return false;
}

/* Destroyed by fire? Make it visible. */

func Incineration()
{
	// GrappleBow becomes unusable on incineration.
	if (hook)
	{
		var rope = hook->GetRope();
		if (rope) rope->BreakRope();
		if (hook) hook->RemoveObject();
	}
	SetClrModulation(0xff606060);
	return _inherited(...);
}

func Extinguishing()
{
	// If extinguished on the same frame it got incinerated, make it usable again
	if (GetCon()>=100)
	{
		EnsureHook();
		SetClrModulation();
	}
	return _inherited(...);
}

/* ++++++++ Animation functions ++++++++ */

public func Reset(clonk)
{
	fAiming = 0;

	clonk->StopAnimation(clonk->GetRootAnimation(11));
	StopAnimation(GetRootAnimation(6));
}

func IsInventorProduct() { return true; }

func Definition(def) {
	SetProperty("PictureTransformation",Trans_Mul(Trans_Translate(-700,400),Trans_Scale(1150),Trans_Rotate(180,0,1,0),Trans_Rotate(-30,-1,0,-1)),def);
}
local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local Collectible = 1;
local Rebuy = true;
local BlastIncinerate = 30;
local ContactIncinerate = 0;
