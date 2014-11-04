// The airplane acts as a container for gold bars and starting material.

#appendto Plane


public func IsContainer() { return true; }

private func MaxContentsCount()
{
	return 25;
}

protected func RejectCollect(id object_id, object obj)
{
	// Objects can collected if gold bar and not above max contents count.
	if (ContentsCount() < MaxContentsCount() && object_id == GoldBar)
		return false;
	return true;
}

// The plane may be broken, which prevents entering it.
local broken;

public func MakeBroken()
{
	broken = true;
	SetEntrance(false);
	return;
}

public func IsBroken()
{
	return broken;
}

public func ActivateEntrance(object clonk)
{
	if (IsBroken())
		return;
	return _inherited(clonk, ...);
}