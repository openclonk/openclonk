/*
	Grapple Help
	Author: Maikel 

	Helper object to let the clonk control the rope with movement keys.
	The clonk is attached to the helper object, which is pulled by the rope. 
	With the movement keys(up/down) the player is able to shorten and lengthen the rope.
	Swinging the rope can be done with left/right.
*/

/*-- Locals + Get/Setters --*/

local bow; // Grapple bow associated with this helper object.
local clonk; // Clonk pulled by the helper.
local rope; // Rope connected to the helper.

public func SetBow(object to_bow)
{
	bow = to_bow;
	// Helper object is inside the bow until the hooks hits solid material.
	Enter(to_bow);
	return;
}

public func SetClonk(object to_clonk)
{
	clonk = to_clonk;
	return;
}

public func GetClonk()
{
	return clonk;
}

public func SetRope(object to_rope)
{
	rope = to_rope;
	return;
}

public func GetRope()
{
	return rope;
}

/*-- Misc --*/

// Called from hook to hang the clonk on the helper object.
public func HangClonkOntoMe()
{
	Exit(0, 10);
	SetXDir(clonk->GetXDir());
	SetYDir(clonk->GetYDir());
	clonk->SetAction("Idle");
	clonk->SetAction("HangOnto", this);
	return;
}

// Called from the rope to notify the helper that the rope snapped.
public func OnRopeBreak()
{
	RemoveObject();
	return;
}

// Called from the clonk to notify the helper that the clonk has let go of the helper.
public func HangOntoLost(object clonk)
{
	if (rope)
		rope->BreakRope();
	RemoveObject();
	return;
}

protected func Destruction()
{
	if (clonk)
	{
		if (this == clonk->GetActionTarget())
		{
			clonk->SetAction("Jump");
			// Pass speed onto clonk.
			clonk->SetXDir(GetXDir());
			clonk->SetYDir(GetYDir());
		}
	}
	return;
}

// Overload of GetMass() to return the clonks mass.
public func GetMass()
{
	if (!clonk) 
		return GetMass();
	return clonk->GetMass();
}

/*-- Grapple rope controls --*/

public func ControlUp()
{
	// Shorten rope.
	if (rope)
	{
		var fxnum = GetEffect("IntGrappleControl", this);
		if (!fxnum)
			fxnum = AddEffect("IntGrappleControl", this, 100, 1, this);
		EffectVar(0, this, fxnum) = true; 
	}
	return true;
}

public func ControlDown()
{
	// Lengthen rope.
	if (rope)
	{
		var fxnum = GetEffect("IntGrappleControl", this);
		if (!fxnum)
			fxnum = AddEffect("IntGrappleControl", this, 100, 1, this);
		EffectVar(1, this, fxnum) = true; 
	}
	return true;
}

public func ControlLeft()
{
	clonk->SetDir(DIR_Left);
	var fxnum = GetEffect("IntGrappleControl", this);
		if (!fxnum)
			fxnum = AddEffect("IntGrappleControl", this, 100, 1, this);
	EffectVar(2, this, fxnum) = true; 
	return true;
}

public func ControlRight()
{
	clonk->SetDir(DIR_Right);
	var fxnum = GetEffect("IntGrappleControl", this);
		if (!fxnum)
			fxnum = AddEffect("IntGrappleControl", this, 100, 1, this);
	EffectVar(3, this, fxnum) = true; 
	return true;
}

public func ControlStop(object clonk, int control)
{
	var fxnum = GetEffect("IntGrappleControl", this);
	if (!fxnum)
		return true;
	if (control == CON_Up) EffectVar(0, this, fxnum) = false;
	if (control == CON_Down) EffectVar(1, this, fxnum) = false; 
	if (control == CON_Left) EffectVar(2, this, fxnum) = false; 
	if (control == CON_Right) EffectVar(3, this, fxnum) = false; 
	return true;
}

// Effect for smooth movement.
public func FxIntGrappleControlTimer(object target, int fxnum)
{
	if (!EffectVar(0, this, fxnum) && !EffectVar(1, this, fxnum)
		&& !EffectVar(2, this, fxnum) && !EffectVar(3, this, fxnum))
		return -1;
	
	// Movement.
	if (EffectVar(0, this, fxnum))
		if (rope)
			rope->DoLength(-1);
	if (EffectVar(1, this, fxnum))
		if (rope)
			rope->DoLength(+1);
	if (EffectVar(2, this, fxnum))
		SetXDir(GetXDir(100) - 5, 100);
	if (EffectVar(3, this, fxnum))
		SetXDir(GetXDir(100) + 5, 100);
	return FX_OK;
}

// Turn when control using objects.
public func ControlUseHolding(object clonk, int x, int y)
{
	return ControlUseTurn(clonk, x, y);
}

public func ControlUseAltHolding(object clonk, int x, int y)
{
	return ControlUseTurn(clonk, x, y);
}

private func ControlUseTurn(object clonk, int x, int y)
{
	if (x > 0)
		if (clonk->GetDir() == DIR_Left)
			clonk->SetDir(DIR_Right);

	if (x < 0)
		if (clonk->GetDir() == DIR_Right)
			clonk->SetDir(DIR_Left);

	return false;
}
