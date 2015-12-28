/**
	Grapple Bow
	A crossbow which is enabled to fire grappling hooks, also has a winching system.
	
	@author Randrian
*/


local is_aiming;
local animation_set;
local hook;
local hook_attach;


private func Initialize()
{
	// The aiming animation is done by adjusting the animation position to fit the angle.
	animation_set = {
		AimMode        = AIM_Position, 
		AnimationAim   = "CrossbowAimArms",
		AnimationShoot = nil,
		ShootTime      = 20,
		TurnType       = 1,
		WalkSpeed      = 84,
		WalkBack       = 56,
		AimSpeed       = 20,
	};
	OnRopeBreak();
	return;
}


/*-- Animations --*/

public func GetCarrySpecial(object clonk) 
{
	if (is_aiming)
		return "pos_hand2";
}

public func GetCarryBone2(object clonk) { return "main2"; }

public func GetCarryMode(object clonk) 
{
	if (hook && !hook->Contained())
		return CARRY_Back;
	if (is_aiming)
		return CARRY_Grappler;
}

public func GetAnimationSet() { return animation_set; }


/*-- Controls --*/

public func HoldingEnabled() { return true; }


public func RejectUse(object clonk)
{
	// Burned?
	if (GetCon() < 100)
		return true;
	// Able to cut the hook? Then never reject the use.
	if (hook->Contained() != this)
		return false;
	return !clonk->HasHandAction();
}

public func ControlUseStart(object clonk, int x, int y)
{
	// Cut rope, or otherwise remove helper object.
	EnsureHook();
	if (hook->Contained() != this)
	{
		var rope = hook->GetRope();
		if (rope)
		{
			rope->DrawIn();
			return true;
		}
		else
		{
			hook->Enter(this);
		}
	}

	// Start aiming.
	is_aiming = true;
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

public func ControlUseHolding(object clonk, int x, int y)
{
	// Update the aiming angle on mouse movement.
	var angle = Angle(0, 0, x, y);
	angle = Normalize(angle, -180);
	angle = BoundBy(angle, -160, 160);
	clonk->SetAimPosition(angle);
	return true;
}

// Stopping says the clonk to stop with aiming (he will go on untill he has finished loading and aiming at the given angle).
public func ControlUseStop(object clonk, int x, int y)
{
	clonk->StopAim();
	return true;
}

// Callback from the clonk, when he actually has stopped aiming.
public func FinishedAiming(object clonk, int angle)
{
	// Only shoot if the bow did not burn in the meantime.
	if (GetCon() < 100)
		return false;
	
	// Shoot the hook and detach the mesh from the bow.
	EnsureHook();
	hook->Exit();
	hook->Launch(angle, 100, clonk, this);
	hook_attach = nil;
	DetachMesh(hook_attach);
	Sound("Objects::Weapons::Bow::Shoot?");

	// Open the hand to let the string go and play the fire animation.
	PlayAnimation("Fire", 6, Anim_Linear(0, 0, GetAnimationLength("Fire"), animation_set["ShootTime"], ANIM_Hold), Anim_Const(1000));
	clonk->StartShoot(this);
	return true;
}

public func ControlUseCancel(object clonk, int x, int y)
{
	clonk->CancelAiming();
	return true;
}


/*-- Bow Mechanics --*/

public func SetHook(object new_hook)
{
	hook = new_hook;
}

private func EnsureHook()
{
	// Create hook if it went missing.
	if (!hook) 
		hook = CreateObject(GrappleHook);
	return hook;
}

public func OnRopeBreak()
{
	if (hook_attach)
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

private func Destruction()
{
	if (hook)
	{
		var rope = hook->GetRope();
		if (rope)
			rope->BreakRope();
	}
}

private func Departure()
{
	if (hook)
	{
		var rope = hook->GetRope();
		if (rope)
			rope->BreakRope();
	}
}

// If shot (e.g. by a cannon) the rope is drawn in.
public func LaunchProjectile()
{
	if (hook)
	{
		var rope = hook->GetRope();
		if (rope)
			rope->DrawIn();
	}
	_inherited(...);
}


/*-- Fire Effects --*/

private func Incineration()
{
	// Grapple bow becomes unusable on incineration.
	if (hook)
	{
		var rope = hook->GetRope();
		if (rope) rope->BreakRope();
		if (hook) hook->RemoveObject();
	}
	SetClrModulation(0xff606060);
	return _inherited(...);
}

private func Extinguishing()
{
	// If extinguished on the same frame it got incinerated, make it usable again
	if (GetCon() >= 100)
	{
		EnsureHook();
		SetClrModulation();
	}
	return _inherited(...);
}


/*-- Animation functions --*/

public func Reset(object clonk)
{
	is_aiming = 0;
	clonk->StopAnimation(clonk->GetRootAnimation(11));
	StopAnimation(GetRootAnimation(6));
}

public func Hit()
{
	Sound("Hits::GeneralHit?");
}

public func IsInventorProduct() { return true; }


/*-- Properties --*/

public func Definition(proplist def)
{
	def.PictureTransformation = Trans_Mul(Trans_Translate(-2500, 1000),Trans_Scale(1800),Trans_Rotate(-60,1,-1,1), Trans_Rotate(180, 0, 1, 0));
}

local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local Collectible = 1;
local BlastIncinerate = 30;
local ContactIncinerate = 0;