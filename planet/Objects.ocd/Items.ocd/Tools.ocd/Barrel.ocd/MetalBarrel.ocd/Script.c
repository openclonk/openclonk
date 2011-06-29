/*--
	Metal Barrel
	Author: Ringwaul

	A sturdier barrel that can transport lava or acid.
--*/

#include Barrel

local szLiquid;
local iVolume;

local debug;


protected func Initialize()
{
	iVolume = 0;
	debug=0;
}

private func Hit()
{
	Sound("Clonk.ogg");
	if(iVolume >= 1) {
		if(GBackLiquid(0,7) && GetMaterial(0,7) != szLiquid) return 0;
		EmptyBarrel(GetR());
		Sound("Splash1");
	}
}

private func FillWithLiquid()
{
	var mat = GetMaterial();

	if(mat == Material("Water"))
	{
		FillBarrel(MaterialName(mat));
		SetMeshMaterial("MB_Water");
		return 1;
	}

	if(mat == Material("Acid"))
	{
		FillBarrel(MaterialName(mat));
		SetMeshMaterial("MB_Acid");
		return 1;
	}

	if(mat == Material("Lava") || mat == Material("DuroLava"))
	{
		FillBarrel(MaterialName(mat));
		SetMeshMaterial("MB_Lava");
		return 1;
	}
}

private func OriginalTex()
{
	SetMeshMaterial("MetalBarrel");
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Rebuy = true;
