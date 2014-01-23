/*--
	Metal Barrel
	Author: Ringwaul, ST-DDT

	A sturdier barrel that can transport lava or acid.
--*/

#include Barrel

private func Hit()
{
	Sound("DullMetalHit?");
	if (iVolume >= 1)
	{
		if (GBackLiquid(0, 7) && GetMaterial(0, 7) != szLiquid)
			return 0;
		EmptyBarrel(GetR());
		Sound("Splash1");
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

local Collectible = false;
local Touchable = 2;
local Name = "$Name$";
local Description = "$Description$";
local Rebuy = true;
local ContactIncinerate = 0;