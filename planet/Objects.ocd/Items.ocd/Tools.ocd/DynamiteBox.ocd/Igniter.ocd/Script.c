/** 
	Dynamite Igniter 
	Can be used to ignite dynamite from a distance.
		
	@author Newton
*/

#include DynamiteBox

private func Hit()
{
	Sound("Hits::Materials::Metal::DullMetalHit?");
}

public func HoldingEnabled() { return true; }

public func GetCarryMode() { return CARRY_BothHands; }

public func GetCarryPhase() { return 250; }

public func GetCarrySpecial(clonk) 
{ 
	if (ignited) 
		return "pos_hand2";
}

local ignited;
local dynamite_sticks;
local wires;

public func ControlUse(object clonk, int x, int y)
{
	if (clonk->GetAction() != "Walk") 
		return true;
	
	ignited = 1;
	
	// The clonk has to stand.
	clonk->SetAction("Stand");
	clonk->SetXDir(0);

	var ignite_time = 40;
	clonk->PlayAnimation("DoIgnite", CLONK_ANIM_SLOT_Arms, Anim_Linear(0, 0, clonk->GetAnimationLength("DoIgnite"), ignite_time, ANIM_Hold), Anim_Const(1000));
	PlayAnimation("Ignite", 1, Anim_Linear(0, 0, GetAnimationLength("Ignite"), ignite_time, ANIM_Hold), Anim_Const(1000));

	ScheduleCall(this, "Ignite", ignite_time, 1, clonk);
	return true;
}

public func Ignite(object clonk)
{
	if (wires[0])
		wires[0]->StartFusing(this);
	else
		for (var obj in FindObjects(Find_Category(C4D_StaticBack), Find_Func("IsFuse"), Find_ActionTargets(this)))
			obj->~StartFusing(this);

	ScheduleCall(this, "ResetClonk", 12, 1, clonk);
	return;
}

public func OnFuseFinished()
{
	if (Contained() != nil) 
		return ResetClonk(Contained());
	return RemoveObject();
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

// Only the main dynamite box is stored.
public func SaveScenarioObject() { return false; }

public func IsTool() { return true; }
public func IsChemicalProduct() { return false; }


/*-- Properties --*/

func Definition(def) {
	SetProperty("PictureTransformation",Trans_Mul(Trans_Rotate(-25, 1, 0, 0), Trans_Rotate(40, 0, 1, 0)), def);
}
local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";

