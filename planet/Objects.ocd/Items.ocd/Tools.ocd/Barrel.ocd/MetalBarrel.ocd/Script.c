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

private func FillWithLiquid()
{
	var mat = GetMaterial();
	// Accept water, acid, lava and oil.
	if (mat == Material("Water") || mat == Material("Acid") || mat == Material("Lava") || mat == Material("DuroLava") || mat == Material("Oil"))
	{
		FillBarrel(MaterialName(mat));
		UpdateBarrel();
	}
	return;	
}

private func UpdateBarrel()
{
	if (iVolume == 0)
		SetMeshMaterial("MetalBarrel");
	else
	{
		if (szLiquid == "Water")
			SetMeshMaterial("MB_Water");
		if (szLiquid == "Acid")
			SetMeshMaterial("MB_Acid");
		if (szLiquid == "Lava" || szLiquid == "DuroLava")
			SetMeshMaterial("MB_Lava");
		if (szLiquid == "Water")
			SetMeshMaterial("MB_Oil");	
	}
	return;
}

public func IsBarrelForMaterial(string sznMaterial)
{
	if ((iVolume > 0) && (WildcardMatch(szLiquid,sznMaterial)))
		return true;
	if (WildcardMatch("Water", sznMaterial))
		return true;
	if (WildcardMatch("Acid", sznMaterial))
		return true;
	if (WildcardMatch("Lava", sznMaterial))
		return true;
	if (WildcardMatch("DuroLava", sznMaterial))
		return true;
	if (WildcardMatch("Oil", sznMaterial))
		return true;
	//if (GetMaterialVal("Density","Material",Material(szMaterial)))
	//	return true;
	return false;
}

local Collectible = false;
local Touchable = 2;
local Name = "$Name$";
local Description = "$Description$";
local Rebuy = true;
local ContactIncinerate = 0;