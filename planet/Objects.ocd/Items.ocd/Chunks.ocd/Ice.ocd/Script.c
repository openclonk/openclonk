/*--- Ice Chunk ---*/

#include Library_CarryHeavy

public func GetCarryMode(clonk) { return CARRY_BothHands; }
public func GetCarryPhase() { return 800; }

protected func Construction()
{
	AddTimer("Check", 30);
}

protected func Check()
{
	if (GetTemperature() <= 0 && GetMaterial() == Material("Water") && GetCon() < 100)
		Freeze();
	// Don't do anything af
	if (GetTemperature() > 0)
		Melt();
}

private func Melt()
{
	CastPXS("Water", 2, 0);
	DoCon(-1);
}

private func Freeze()
{
	ExtractMaterialAmount(0, 0, Material("Water"), 2);
	DoCon(1);
}

protected func Hit()
{
	Sound("CrystalHit?");
}

public func IsChunk() { return true; }

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(30,0,0,1),Trans_Rotate(-30,1,0,0),Trans_Scale(1300)),def);
}

local Name = "$Name$";
local Description = "$Description$";
local Touchable = 2;
local Plane = 450;