/*
	Grapple Bow
	Author: Randrian

	A crossbow which is enabled to fire grappling hooks, also has a winching system.
*/

local fAiming;

local hook;
local hook_attach;

public func GetCarryMode() {  if(hook->Contained() == nil) return CARRY_Back; return CARRY_HandBack; }

public func GetCarrySpecial(clonk) { if(fAiming) return "pos_hand2"; }
public func GetCarryBone2(clonk) { return "main2"; }
public func GetCarryMode(clonk) { if(hook->Contained() == nil) return CARRY_Back; if(fAiming >= 0) return CARRY_Grappler; }

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
		WalkSpeed      = 30000,
		WalkBack       = 20000,
		AimSpeed       = 20,            // the speed of aiming
	};
	OnRopeBreak();
}

public func SetHook(object new_hook)
{
	hook = new_hook;
}

public func OnRopeBreak()
{
	if(hook_attach)
		DetachMesh(hook_attach);

	hook = CreateObject(GrappleHook, 0, 0, NO_OWNER);
	hook->Enter(this);
	hook_attach = AttachMesh(hook, "bolt", "main");
	PlayAnimation("Load", 5, Anim_Const(GetAnimationLength("Load")), Anim_Const(1000));
}

protected func Destruction()
{
	var rope = hook->GetRope();
	if (rope)
		rope->BreakRope();
}

protected func Departure()
{
	var rope = hook->GetRope();
	if (rope)
		rope->DrawIn();
}

public func GetAnimationSet() { return animation_set; }

public func ControlUseStart(object clonk, int x, int y)
{
	// Cut rope, or otherwise remove helper object.
	if (hook->Contained() != this)
	{
		var rope = hook->GetRope();
		if (rope)
		{
			rope->DrawIn();
		//	rope->BreakRope();
		}
		return true;
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
	DetachMesh(hook_attach);
	hook_attach = nil;

	hook->Exit();
	hook->Launch(angle, 100, clonk, this);
	DetachMesh(hook_attach);
	Sound("BowShoot*.ogg");

	// Open the hand to let the string go and play the fire animation
	PlayAnimation("Fire", 6, Anim_Linear(0, 0, GetAnimationLength("Fire"), animation_set["ShootTime"], ANIM_Hold), Anim_Const(1000));
	clonk->StartShoot(this);
	return true;
}

public func ControlUseCancel(object clonk, int x, int y)
{
	clonk->CancelAiming(this);
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

/* ++++++++ Animation functions ++++++++ */

public func Reset(clonk)
{
	fAiming = 0;

	clonk->StopAnimation(clonk->GetRootAnimation(11));
	StopAnimation(GetRootAnimation(6));
}

func Definition(def) {
	SetProperty("PictureTransformation",Trans_Mul(Trans_Translate(-700,400),Trans_Scale(1150),Trans_Rotate(180,0,1,0),Trans_Rotate(-30,-1,0,-1)),def);
}
local Name = "$Name$";
local Description = "$Description$";
