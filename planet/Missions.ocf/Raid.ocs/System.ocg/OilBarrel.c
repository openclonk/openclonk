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
		var plane = FindObject(Find_ID(Airplane), Find_AtPoint());
		if (plane)
		{
			ScheduleCall(nil, Global.GameCall, 1,1, "OnPlaneLoaded", plane, this);
		}
	}
	return inherited(...);
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

