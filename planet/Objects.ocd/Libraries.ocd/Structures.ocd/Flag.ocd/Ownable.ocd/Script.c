/**
	Ownable Library
	Must be included by structures and handles the ownership when inside 
	the reign of a flagpole.
	
	@author Zapper, Maikel
*/


protected func Initialize()
{
	// Set the right owner based on the flag's ownership radiuses.
	SetOwner(GetOwnerOfPosition(0, 0));
	// A movement check is needed because structures can move out of flag radiuses.
	AddEffect("IntMovementCheck", this, 100, 12, this);
	return _inherited(...);
}

// This object is affected by ownership radiuses.
public func CanBeOwned() 
{ 
	// Flagpoles should not affect the ownership of other flagpoles, so
	// return false for objects which include both the flag and ownable
	// library. Return true otherwise.
	return !this->~IsFlagpole();
}

// Callback from the engine: 
public func OnOwnerChanged(int new_owner, int old_owner)
{
	// ...
	return _inherited(new_owner, old_owner, ...);
}

// Restrict interactions to allies of the structure's owner.
public func IsInteractable(object clonk)
{
	if (Hostile(GetOwner(), clonk->GetOwner())) 
		return false;
	return _inherited(clonk, ...);
}

// And show a message in an interaction menu.
public func RejectInteractionMenu(object clonk)
{
	if (Hostile(GetOwner(), clonk->GetOwner())) 
		return Format("$MsgHostile$", GetName(), GetTaggedPlayerName(GetOwner()));
	return _inherited(clonk, ...);
}

/*-- Movement Check --*/

protected func FxIntMovementCheckStart(object target, proplist effect, int temp)
{
	if (temp)
		return FX_OK;
	effect.Interval = 9;
	// Store initial x and y.
	effect.x = GetX();
	effect.y = GetY();
	return FX_OK;
}

protected func FxIntMovementCheckTimer(object target, proplist effect)
{
	// Remove effect for structures that can't be owned.
	if (!this->~CanBeOwned())
		return FX_Execute_Kill;	
	// Check whether the structure has moved.
	if (GetX() != effect.x || GetY() != effect.y)
	{
		// Determine if owner has changed.
		var new_owner = GetOwnerOfPosition(0, 0);
		if (GetOwner() != new_owner)
			SetOwner(new_owner);
		// Update new x and y.
		effect.x = target->GetX();
		effect.y = target->GetY();
	}
	return FX_OK;	
}
