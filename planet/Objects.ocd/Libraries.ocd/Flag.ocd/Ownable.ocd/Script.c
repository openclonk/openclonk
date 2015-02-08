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
	return _inherited(...);
}

// This object is affected by ownership radiuses.
public func CanBeOwned() { return true; }

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
