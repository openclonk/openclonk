/**
	CarryHeavyControl

	Handles the control of carry-heavy-objects as inventory objects.
	Depends on Clonk Control and Inventory
*/

local lib_carryheavy_obj; // object beeing carried with carryheavy

public func GetCarryHeavy() { return lib_carryheavy_obj; }
public func IsCarryingHeavy() { return lib_carryheavy_obj != nil; }


// Helper function to create carry heavy contents without doing the pick up animation.
public func CreateCarryHeavyContents(id obj_id, int amount)
{
	this.BlockCarryHeavyPickUpAnimation = true;
	var res = CreateContents(obj_id, amount);
	this.BlockCarryHeavyPickUpAnimation = false;
	return res;
}

/* Overloads for Inventory */

// Check if we can carry a carry heavy object
protected func RejectCollect(id objid, object obj)
{
	// Carry heavy only gets picked up if none held already
	if(this.inventory.force_collection && obj->~IsCarryHeavy())
	{
		// Collection of that object magically disabled?
		if(GetEffect("NoCollection", obj)) return true;
		
		// Do callbacks to control effects to see if the effect blocks picking up a carry heavy object.
		var block_carry_heavy = false;
		var count = GetEffectCount("*Control*", this), control_effect;
		while (count--)
		{
			control_effect = GetEffect("*Control*", this, count);
			if (control_effect && EffectCall(this, control_effect, "RejectCarryHeavyPickUp", obj))
			{
				block_carry_heavy = true;
				break;
			}
		}

		// Don't pick up if already carrying a heavy object or if it is blocked.
		if (IsCarryingHeavy() || block_carry_heavy)
		{
			CustomMessage("$TxtHandsFull$", this, this->GetController(), 0, 0, 0xff0000);
			return true;
		}
		return false;
	}

	return _inherited(objid,obj,...);
}

// Start visual effect when picking up carry heavy objects
public func Collection2(object obj)
{
	if(obj->~IsCarryHeavy()) // we can assume that we don't have a carryheavy object yet. If we do, Scripters are to blame.
	{
		CarryHeavy(obj);
		DoLiftCarryHeavy(obj);
		return true;
	}
	return _inherited(obj,...);
}

public func GetHandItem(int i)
{
	// carrying a carry heavy item always returns said item. (he holds it in both hands, after all!)
	if (IsCarryingHeavy())
		return GetCarryHeavy();
	return _inherited(i, ...);
}

// Delete internal variables
public func Ejection(object obj)
{
	// carry heavy special treatment
	if(obj == GetCarryHeavy())
	{
		StopCarryHeavy();
		return true;
	}
	return _inherited(obj,...);
}

// Delete internal variables
public func ContentsDestruction(object obj)
{
	// check if it was carryheavy
	if(obj == GetCarryHeavy())
	{
		StopCarryHeavy();
	}

	return _inherited(obj, ...);
}

/* Overloads for Clonk Control */

public func GetExtraInteractions()
{
	var functions = _inherited(...);

	// dropping carry heavy
	if (IsCarryingHeavy() && GetAction() == "Walk")
	{
		var ch = GetCarryHeavy();
		PushBack(functions, {Fn = "DropCarryHeavy", Description=ch->GetDropDescription(), Object=this, IconName="", IconID=Icon_LetGo, Priority=1});
	}
	return functions;
}

/* Carry heavy stuff */

/** Tells the clonk that he is carrying the given carry heavy object */
public func CarryHeavy(object target)
{
	if (!target)
		return;
	// actually.. is it a carry heavy object?
	if (!target->~IsCarryHeavy())
		return;
	// only if not carrying a heavy object already
	if (IsCarryingHeavy())
		return;

	lib_carryheavy_obj = target;

	if(target->Contained() != this)
		target->Enter(this);

	// Update attach stuff
	this->~OnSlotFull();
	
	// Do callbacks to control effects for this clonk that a carry heavy object has been picked up.
	var count = GetEffectCount("*Control*", this), control_effect;
	while (count--)
	{
		control_effect = GetEffect("*Control*", this, count);
		if (control_effect)
			EffectCall(this, control_effect, "OnCarryHeavyPickUp", target);
	}

	return true;
}

/** Drops the carried heavy object, if any */
public func DropCarryHeavy()
{
	// only if actually possible
	if(!IsCarryingHeavy())
		return;

	DoDropCarryHeavy(GetCarryHeavy());
	StopCarryHeavy();

	return true;
}

// Internal function to clear carryheavy-status
private func StopCarryHeavy()
{
	if(!IsCarryingHeavy())
		return;

	lib_carryheavy_obj = nil;
	this->~OnSlotEmpty();
}

/* Lifting / dropping effect */

local lib_carryheavy_lifttime = 22;

private func DoLiftCarryHeavy(object obj)
{
	if (!obj || !obj->~IsCarryHeavy())
		return;
	if (obj->Contained() != this)
		return;
	// If inside something or not walking, skip the animation
	if (Contained() || GetAction() != "Walk" || this.BlockCarryHeavyPickUpAnimation)
		return;
	AddEffect("IntLiftHeavy", this, 1, 1, this, nil, obj);
}

private func DoDropCarryHeavy(object obj)
{
	if (!obj || !obj->~IsCarryHeavy())
		return;
	if (obj->Contained() != this)
		return;
	// If inside something or not walking, skip the animation
	if (Contained() || GetAction() != "Walk")
		return;
	AddEffect("IntDropHeavy", this, 1, 1, this, nil, obj);
}

private func FxIntLiftHeavyStart(object me, proplist effect, bool tmp, object to_lift)
{
	//Stop the clonk from moving, and tell the clonk's control library
	//it now has a hand action
	me->SetTurnForced(GetDir());
	me->SetHandAction(1);
	SetAction("Stand");
	//Stop the clonk if he is moving
	if(GetXDir() != 0) SetXDir();

	//Attach the mesh of the object. It is not displayed normally because the
	//hands are told they have an action in the next few lines
	effect.mesh = AttachMesh(to_lift, "pos_tool1", to_lift->GetCarryBone(), to_lift->~GetCarryTransform(this));

	//Play the animation of the clonk picking up the object
	effect.anim = PlayAnimation("CarryArmsPickup", CLONK_ANIM_SLOT_Arms, Anim_Linear(0,0, GetAnimationLength("CarryArmsPickup"), lib_carryheavy_lifttime, ANIM_Remove), Anim_Const(1000));

	effect.obj = to_lift;
}

private func FxIntLiftHeavyTimer(object me, proplist effect, int time)
{
	//If the clonk moves, he'll stop lifting and drop the object
	if(time < lib_carryheavy_lifttime)
	{
		// first 2/3 of time, clonk will drop the object
		if(time < lib_carryheavy_lifttime * 2 / 3)
		{
			if(GetAction() != "Stand" || me->IsJumping() || Abs(GetXDir()) > 0)
			{
				effect.fail = true;
				return FX_Execute_Kill;
			}
		}
		else
		{
			if(GetAction() != "Stand")
				return FX_Execute_Kill;
		}
	}
	//When the clonk has finished lifting, remove movement-restrictions
	if(time >= lib_carryheavy_lifttime)
		return FX_Execute_Kill;
	// Object got moved out during lifting
	if(!effect.obj || effect.obj->Contained() != this)
		return FX_Execute_Kill;
}

private func FxIntLiftHeavyStop(object me, proplist effect)
{
	// If failed, drop the object
	if(effect.fail && effect.obj && effect.obj->Contained() == this)
		effect.obj->Exit(0,9);

	DetachMesh(effect.mesh);
	StopAnimation(effect.anim);
	me->SetTurnForced(-1);
	me->SetHandAction(0);
	if(GetAction() == "Stand") SetAction("Walk");
}

private func FxIntDropHeavyStart(object me, proplist effect, bool tmp, object to_lift)
{
	if(GetEffect("IntCarryHeavy"))
		RemoveEffect("IntCarryHeavy", this, nil, true);

	//Stop the clonk from moving, and tell the clonk's control library
	//it now has a hand action
	me->SetTurnForced(GetDir());
	me->SetHandAction(1);
	SetAction("Stand");
	//Stop the clonk if he is moving
	if(GetXDir() != 0) SetXDir();

	//Attach the mesh of the object. It is not displayed normally because the
	//hands are told they have an action in the next few lines
	effect.mesh = AttachMesh(to_lift, "pos_tool1", to_lift->GetCarryBone(), to_lift->~GetCarryTransform(this));

	//Play the animation of the clonk setting down the object
	effect.anim = PlayAnimation("CarryArmsPickup", CLONK_ANIM_SLOT_Arms, Anim_Linear(GetAnimationLength("CarryArmsPickup"), GetAnimationLength("CarryArmsPickup"), 0, lib_carryheavy_lifttime, ANIM_Remove), Anim_Const(1000));

	effect.obj = to_lift;
}

private func FxIntDropHeavyTimer(object me, proplist effect, int time)
{
	if (time >= lib_carryheavy_lifttime)
		return FX_Execute_Kill;
	if (GetAction() != "Stand")
		return FX_Execute_Kill;
	if (!effect.obj)
		return FX_Execute_Kill;
	if (effect.obj->Contained() != this)
		return FX_Execute_Kill;
}

private func FxIntDropHeavyStop(object me, proplist effect)
{
	// Drop the object
	if(effect.obj && effect.obj->Contained() == this)
		effect.obj->Exit(0,9);

	DetachMesh(effect.mesh);
	StopAnimation(effect.anim);
	me->SetTurnForced(-1);
	me->SetHandAction(0);
	if(GetAction() == "Stand") SetAction("Walk");
}