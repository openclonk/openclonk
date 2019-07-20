// Goal timer: Barrel has to be brought to the plane
// Doesn't matter if contained or not.

#appendto MetalBarrel

public func Initialize(...)
{
	AddTimer(this.CheckForPlane, 20);
	return _inherited(...);
}

private func CheckForPlane()
{
	var plane = FindObject(Find_ID(Airplane), Find_AtPoint());
	if (plane)
	{
		ScheduleCall(nil, Global.GameCall, 1, 1, "OnPlaneLoaded", plane, this);
		RemoveTimer(this.CheckForPlane);
	}
}

// The barrel is always full with oil and does not accept anything else.
private func AcceptMaterial(int material)
{
	return false;
}

// ...and can also not be emptied.
public func RejectUse()
{
	return true;
}

// ...not even when it hits the ground
public func Hit()
{
	this->PlayBarrelHitSound();
}
// (okay, you could still put it into a building and attach a pump)

