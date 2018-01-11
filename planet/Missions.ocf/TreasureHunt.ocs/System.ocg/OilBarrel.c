// Goal timer: Barrel has to be brought to the plane
// Doesn't matter if container or not.

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
		ScheduleCall(nil, Global.GameCall, 1,1, "OnPlaneLoaded", plane, this);
		RemoveTimer(this.CheckForPlane);
	}
}

