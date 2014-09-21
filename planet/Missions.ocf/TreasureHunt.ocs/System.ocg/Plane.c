#appendto Plane

// plane needs oil
public func ContainedUp(object clonk, ...)
{
	if (!FuelCheck(clonk)) return true;
	return _inherited(clonk, ...);
}

public func ContainedDown(object clonk, ...)
{
	if (!FuelCheck(clonk)) return true;
	return _inherited(clonk, ...);
}

func FuelCheck(object clonk)
{
	if (!FindContents(MetalBarrel))
	{
		Dialogue->MessageBox("$PlaneNoOil$", clonk, clonk);
		clonk->Sound("WipfWhine");
		return false;
	}
	return true;
}
