#appendto Airplane

func Initialize(...)
{
	// (first) plane built? Story advances.
	// only after attack, so enemy planes don't count
	if (g_attack_done && !g_plane_built)
	{
		g_plane_built = true;
		var closest_clonk = FindObject(Find_ID(Clonk), Find_Not(Find_Owner(NO_OWNER)), Sort_Distance());
		SetColor(0xa04000); // Make sure it has the same color in all missions
		Dialogue->MessageBoxAll("$PlaneBuilt$", closest_clonk, true);
		this.Touchable = 0;
	}
	return _inherited(...);
}

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
