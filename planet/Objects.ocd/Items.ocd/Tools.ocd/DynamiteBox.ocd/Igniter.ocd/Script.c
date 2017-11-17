/** 
	Dynamite Igniter
	Can be used to ignite dynamite from a distance.
	
	@author Newton
*/

#include DynamiteBox


/*-- Engine Callbacks --*/

public func Hit()
{
	Sound("Hits::Materials::Metal::DullMetalHit?");
}


/*-- Callbacks --*/

public func OnFuseFinished()
{
	if (Contained() != nil) 
		return ResetClonk(Contained());
	return RemoveObject();
}

/*-- Usage --*/

public func HoldingEnabled() { return true; }

public func ControlUseStart(object clonk, int x, int y)
{
	if (clonk->GetAction() != "Walk") 
		return true;
	if (IsBeingIgnited())
		return true;
		
	var ignite_time = 40;
	// Launch the ignition effect.
	CreateEffect(FxScheduleIgnite, 100, ignite_time, clonk);

	// The clonk has to stand.
	clonk->SetAction("Stand");
	clonk->SetXDir(0);

	// Play ignite animation.
	clonk->PlayAnimation("DoIgnite", CLONK_ANIM_SLOT_Arms, Anim_Linear(0, 0, clonk->GetAnimationLength("DoIgnite"), ignite_time, ANIM_Hold), Anim_Const(1000));
	PlayAnimation("Ignite", 1, Anim_Linear(0, 0, GetAnimationLength("Ignite"), ignite_time, ANIM_Hold));
	return true;
}

public func ControlUseStop(object clonk, int x, int y)
{
	var fx = GetEffect("FxScheduleIgnite", this);
	if (fx)
		fx->Remove();
	return true;
}

public func ControlUseCancel(object clonk, int x, int y)
{
	var fx = GetEffect("FxScheduleIgnite", this);
	if (fx)
		fx->Remove();
	return true;
}

local FxScheduleIgnite = new Effect
{
	Construction = func(object clonk)
	{
		this.clonk = clonk;
	},
	Timer = func()
	{
		Target->Ignite(this.clonk);
		return FX_Execute_Kill;	
	},
	Destruction = func()
	{
		Target->ResetClonk(this.clonk, false);
	}
};

public func IsBeingIgnited()
{
	return !!GetEffect("FxScheduleIgnite", this);
}

public func Ignite(object clonk)
{
	// Ignite all connected wires
	for (var obj in FindFuses())
		obj->~StartFusing(this);
	ScheduleCall(this, "ResetClonk", 4, 1, clonk, true);
	return;
}

public func ResetClonk(object clonk, bool remove_igniter)
{
	// Reset animation of the clonk.
	clonk->StopAnimation(clonk->GetRootAnimation(CLONK_ANIM_SLOT_Arms));
	clonk->SetAction("Walk");
	if (remove_igniter)
		clonk->DetachObject(this);
	// Reset animation of the igniter and remove it.
	StopAnimation(GetRootAnimation(1));
	if (remove_igniter)
		RemoveObject();
	return;
}


/*-- Production --*/

public func IsTool() { return true; }
public func IsChemicalProduct() { return false; }


/*-- Display --*/

public func GetCarryMode()
{
	return CARRY_BothHands;
}

public func GetCarryPhase() { return 250; }

public func GetCarryTransform()
{
	if (IsBeingIgnited())
		return Trans_Mul(Trans_Rotate(0, 1), Trans_Translate(-1000));
}

public func GetCarrySpecial()
{ 
	if (IsBeingIgnited())
		return "pos_hand2";
}

public func Definition(proplist def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(-25, 1, 0, 0), Trans_Rotate(40, 0, 1, 0)), def);
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;