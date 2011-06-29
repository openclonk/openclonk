local Name = "$Name$";
local Description = "$Description$";

public func CanBeOwned(){return true;}

public func OnOwnerChanged(int old_owner)
{
	// ...
}

public func IsInteractable(object clonk)
{
	if(Hostile(GetOwner(), clonk->GetOwner())) return false;
	return _inherited(clonk, ...);
}