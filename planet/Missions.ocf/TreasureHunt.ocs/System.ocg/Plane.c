#appendto Airplane

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
		clonk->Sound("Animals::Wipf::Whine");
		return false;
	}
	return true;
}
