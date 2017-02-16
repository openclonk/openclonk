/*--
	Metal Barrel
	Author: Ringwaul, ST-DDT

	A sturdier barrel that can transport lava or acid.
--*/

#include Barrel

func PlayBarrelHitSound()
{
	Sound("Hits::Materials::Metal::DullMetalHit?");
}

public func IsLiquidContainerForMaterial(string liquid_name)
{
	// anything liquid
	var density = GetMaterialVal("Density", "Material", Material(liquid_name));
	return _inherited(liquid_name, ...) || (density < 50 && density >= 25);
}



local LiquidNames = {
	Acid = "$MaterialAcid$",
	DuroLava = "$MaterialDuroLava$",
	Firefluid = "$MaterialFirefluid$",
	Lava = "$MaterialLava$",
	Oil = "$MaterialOil$",
	Water = "$MaterialWater$",
};


local Name = "$Name$";
local Description = "$Description$";
local ContactIncinerate = 0;
local Components = {Metal = 2};
