#appendto MetalBarrel

func Exit(...)
{
	// dropping at plane? then put into plane
	if (Contained() && Contained()->GetAlive())
	{
		var plane = FindObject(Find_ID(Airplane), Find_AtPoint());
		if (plane)
		{
			ScheduleCall(nil, Global.GameCall, 1,1, "OnPlaneLoaded", plane, this);
		}
	}
	return inherited(...);
}