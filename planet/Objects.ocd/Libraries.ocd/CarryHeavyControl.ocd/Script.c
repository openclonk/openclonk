/**
	CarryHeavyControl
	Handles the control of carry-heavy-objects as inventory objects.
*/

local carryheavy; // object beeing carried with carryheavy

// overload for carry-heavy
public func GetHandItem(int i)
{
	// carrying a carry heavy item always returns said item. (he holds it in both hands, after all!)
	if (IsCarryingHeavy())
		return GetCarryHeavy();
	return _inherited(i, ...);
}

// overload for carry-heavy
public func SetHandItemPos(int hand, int inv)
{
	// can't use anything except carryheavy if carrying heavy object.
	if(IsCarryingHeavy())
		return nil;
	return _inherited(hand, inv, ...);
}

func Collection2(object obj)
{
	// carryheavy object gets special treatment
	if(obj->~IsCarryHeavy()) // we can assume that we don't have a carryheavy object yet. If we do, Scripters are to blame.
	{
		if(obj != GetCarryHeavy())
			CarryHeavy(obj);
		
		return true;
	}
	return _inherited(obj,...);
}

func Ejection(object obj)
{
	// carry heavy special treatment
	if(obj == GetCarryHeavy())
	{
		StopCarryHeavy();
		return true;
	}
	_inherited(obj,...);
}

func ContentsDestruction(object obj)
{
	// check if it was carryheavy
	if(obj == GetCarryHeavy())
	{
		StopCarryHeavy();
	}

	_inherited(obj, ...);
}

protected func RejectCollect(id objid, object obj)
{
	// collection of that object magically disabled?
	if(GetEffect("NoCollection", obj)) return true;

	// Carry heavy only gets picked up if none held already
	if(obj->~IsCarryHeavy())
	{
		if(IsCarryingHeavy())
			return true;
		else
		{
			return false;
		}
	}

	//No collection if the clonk is carrying a 'carry-heavy' object
	// todo: does this still make sense with carry heavy not beeing in inventory and new inventory in general?
	if(IsCarryingHeavy() && !this.inventory.force_collection) return true; // this.inventory.force_collection declared in Inventory.ocd
	return _inherited(objid,obj,...);
}

// overload function from ClonkControl
public func GetExtraInteractions()
{
	var functions = _inherited(...);
		
	// dropping carry heavy
	if(IsCarryingHeavy() && GetAction() == "Walk")
	{
		var ch = GetCarryHeavy();
		PushBack(functions, {Fn = "Drop", Description=ch->GetDropDescription(), Object=ch, IconName="", IconID=Icon_LetGo, Priority=1});
	}
	return functions;
}

/* Carry heavy stuff */

/** Tells the clonk that he is carrying the given carry heavy object */
public func CarryHeavy(object target)
{
	if(!target)
		return;
	// actually.. is it a carry heavy object?
	if(!target->~IsCarryHeavy())
		return;
	// only if not carrying a heavy objcet already
	if(IsCarryingHeavy())
		return;	
	
	carryheavy = target;
	
	if(target->Contained() != this)
		target->Enter(this);

	// Update attach stuff
	this->~OnSlotFull();
	
	return true;
}

/** Drops the carried heavy object, if any */
public func DropCarryHeavy()
{
	// only if actually possible
	if(!IsCarryingHeavy())
		return;
	
	GetCarryHeavy()->Drop();
	StopCarryHeavy();
	
	return true;
}

// Internal function to clear carryheavy-status
private func StopCarryHeavy()
{
	if(!IsCarryingHeavy())
		return;
	
	carryheavy = nil;
	this->~OnSlotEmpty();
}

public func GetCarryHeavy() { return carryheavy; }
public func IsCarryingHeavy() { return carryheavy != nil; }

public func ObjectControl(int plr, int ctrl, int x, int y, int strength, bool repeat, bool release)
{
	if (!this) 
		return false;
	var handled = _inherited(plr, ctrl, x, y, strength, repeat, release, ...);
	if(handled) return handled;
	
	if(ctrl == CON_Interact)
	{
		if(IsCarryingHeavy() && GetAction() == "Walk")
		{
			DropCarryHeavy();
			return true;
		}
	}
}