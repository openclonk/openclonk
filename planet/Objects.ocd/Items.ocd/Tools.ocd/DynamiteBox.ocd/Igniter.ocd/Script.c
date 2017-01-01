/** 
	Dynamite Igniter
	Can be used to ignite dynamite from a distance.
	
	@author Newton
*/

#include DynamiteBox

local ignited;

/*-- Engine Callbacks --*/

func Hit()
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

public func ControlUse(object clonk, int x, int y)
{
	if (clonk->GetAction() != "Walk") 
		return true;
	if (ignited)
		return true;

	ignited = true;

	// The clonk has to stand.
	clonk->SetAction("Stand");
	clonk->SetXDir(0);

	var ignite_time = 40;
	clonk->PlayAnimation("DoIgnite", CLONK_ANIM_SLOT_Arms, Anim_Linear(0, 0, clonk->GetAnimationLength("DoIgnite"), ignite_time, ANIM_Hold), Anim_Const(1000));
	PlayAnimation("Ignite", 1, Anim_Linear(0, 0, GetAnimationLength("Ignite"), ignite_time, ANIM_Hold));

	ScheduleCall(this, "Ignite", ignite_time, 1, clonk);
	return true;
}

public func Ignite(object clonk)
{
	// Ignite all connected wires
	for (var obj in FindFuses())
		obj->~StartFusing(this);

	ScheduleCall(this, "ResetClonk", 12, 1, clonk);
	return;
}

public func ResetClonk(object clonk)
{
	// Reset animation of the clonk.
	clonk->StopAnimation(clonk->GetRootAnimation(10));
	clonk->SetAction("Walk");
	clonk->DetachObject(this);
	// Reset animation of the igniter and remove it.
	StopAnimation(GetRootAnimation(1));
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
	if (ignited)
		return Trans_Mul(Trans_Rotate(0, 1), Trans_Translate(-1000));
}

public func GetCarrySpecial()
{ 
	if (ignited)
		return "pos_hand2";
}

func Definition(def)
{
	SetProperty("PictureTransformation",Trans_Mul(Trans_Rotate(-25, 1, 0, 0), Trans_Rotate(40, 0, 1, 0)), def);
}

/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;