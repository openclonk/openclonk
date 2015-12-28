/*--
	Metal Barrel
	Author: Ringwaul, ST-DDT

	A sturdier barrel that can transport lava or acid.
--*/

#include Barrel

private func Hit()
{
	Sound("Hits::Materials::Metal::DullMetalHit?");
	if (iVolume >= 1)
	{
		if (GBackLiquid(0, 7) && GetMaterial(0, 7) != szLiquid)
			return 0;
		EmptyBarrel(GetR());
		Sound("Liquids::Splash1");
	}
}

private func AcceptMaterial(int material)
{
	// Accepts all fluids
	return true;
}

public func IsBarrelForMaterial(string sznMaterial)
{
	// anything liquid
	var density = GetMaterialVal("Density","Material",Material(sznMaterial));
	return density < 50 && density >= 25;
}

local Name = "$Name$";
local Description = "$Description$";
local ContactIncinerate = 0;