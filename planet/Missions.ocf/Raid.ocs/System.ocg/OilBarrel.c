#appendto MetalBarrel

func Entrance(clonk, ...)
{
	if (clonk && clonk->GetAlive() && !clonk.had_oil_message)
	{
		Dialogue->MessageBox("$OilBarrelMsg$", clonk, clonk);
		clonk.had_oil_message = true;
	}
	return _inherited(clonk, ...);
}

func Exit(...)
{
	// dropping at plane? then put into plane
	if (Contained() && Contained()->GetAlive())
	{
		var plane = FindObject(Find_ID(Plane), Find_AtPoint());
		if (plane)
		{
			ScheduleCall(nil, Global.GameCall, 1,1, "OnPlaneLoaded", plane, this);
		}
	}
	return inherited(...);
}