public func CanBeOwned(){return true;}

public func OnOwnerChanged(int new_owner, int old_owner)
{
	// ...
	return _inherited(new_owner, old_owner, ...);
}

public func IsInteractable(object clonk)
{
	if(Hostile(GetOwner(), clonk->GetOwner())) return false;
	return _inherited(clonk, ...);
}

func Initialize()
{
	// set right owner
	SetOwner(GetOwnerOfPosition(0, 0));
	return _inherited(...);
}