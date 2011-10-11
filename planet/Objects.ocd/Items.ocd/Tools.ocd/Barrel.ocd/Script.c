/*--
	Wooden Barrel
	Author: Ringwaul, ST-DDT

	The barrel is used to transport liquids
--*/

local szLiquid;
local iVolume;

local debug;

public func GetCarryMode(clonk)
{
	return CARRY_BothHands;
}
public func GetCarryTransform(clonk)
{
	return Trans_Translate(-1000, -800, 0);
}
public func GetCarryPhase()
{
	return 900;
}

protected func Initialize()
{
	iVolume = 0;
	debug = 0;
}

private func Hit()
{
	Sound("WoodHit");
	if (iVolume >= 1)
	{
		if (GBackLiquid(0, 3) && GetMaterial(0, 3) != szLiquid)
			return 0;
		EmptyBarrel(GetR());
		Sound("Splash1");
	}
}

private func Check()
{
	//Fills Barrel with specified liquid from if submerged
	var iSource = 3;
	
	if (GBackLiquid(0, iSource))
	{
		FillWithLiquid();
	}
	
	if (iVolume == 0)
	{
		SetGraphics();
		szLiquid = nil;
	}
	
	//Value. Base value is 10.
	if (iVolume == 0)
		SetProperty("Value", 10);
	//if(szLiquid == Oil)	SetProperty("Value", 10 + (iVolume / 15)); //No oil in current build
	
	//Debug/Testing Purposes
	if (debug == 1)
		Message("Volume:|%d|Liquid:|%s", iVolume, szLiquid);
}

//over-ridden with metal barrel
private func FillWithLiquid()
{
	var mat = GetMaterial();
	
	if (mat == Material("Water"))
	{
		FillBarrel(MaterialName(mat));
		SetMeshMaterial("Barrel_Water");
	}
}

private func FillBarrel(string szMat)
{
	var iCapacity = BarrelMaxFillLevel();
	var intake = 3;
	
	if (iVolume >= 1 && szMat != szLiquid)
		return 0;
	while (iVolume != iCapacity && GetMaterial(0, intake) == Material(szMat))
	{
		ExtractLiquid(0, intake);
		iVolume = ++iVolume;
	}
	szLiquid = szMat;
}

private func EmptyBarrel(int iAngle, int iStrength)
{
	if (!iAngle)
		iAngle = 0;
	if (!iStrength)
		iStrength = 30;
	CastPXS(szLiquid, iVolume, iStrength, 0, 0, iAngle, 30);
	iVolume = 0;
	OriginalTex();
}

//over-ridden with metal barrel
private func OriginalTex()
{
	SetMeshMaterial("Barrel");
}

public func ControlUse(object clonk, int iX, int iY)
{
	var AimAngle = Angle(0, 0, iX, iY);
	if (iVolume >= 1)
	{
		EmptyBarrel(AimAngle, 50);
		if (iX > 1)
			Contained()->SetDir(1);
		if (iX < -1)
			Contained()->SetDir(0);
	}
	return 1;
}

public func IsToolProduct() { return true; }

public func Definition(proplist def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(0, 1000, 0), Trans_Rotate(-40, 1, 0, 0), Trans_Rotate(20, 0, 0, 1)), def);
}



public func BarrelMaxFillLevel()
{
	return 300;
}

public func GetFillLevel()
{
	return iVolume;
}

public func IsBarrel()
{
	return true;
}

public func BarrelIsEmpty()
{
	return iVolume == 0;
}

public func BarrelIsFull()
{
	return iVolume == BarrelMaxFillLevel();
}

//returns the contained liquid
public func GetBarrelMaterial()
{
	if (iVolume == 0)
		return "";
	return szLiquid;
}

public func IsBarrelForMaterial(string sznMaterial)
{
	return WildcardMatch("Water",sznMaterial);
}

/**
Extract liquid from barrel
@param sznMaterial: Material to extract; Wildcardsupport
@param inMaxAmount: Max Amount of Material being extracted 
@param pnTarget: Object which extracts the liquid
@return [irMaterial,irAmount]
	-irMaterial: Material being extracted
	-irAmount: Amount being extracted
*/
public func GetLiquid(string sznMaterial, int inMaxAmount, object pnTarget)
{
	//Wrong material?
	if (!WildcardMatch(szLiquid, sznMaterial))
		inMaxAmount = 0;
	inMaxAmount = Min(inMaxAmount, iVolume);
	iVolume -= inMaxAmount;
	return [szLiquid, inMaxAmount];
}

/** 
Insert liquid to barrel
	@param sznMaterial: Material to insert
	@param inMaxAmount: Max Amount of Material being inserted 
	@param pnSource: Object which inserts the liquid
	@return inAmount: The inserted amount
*/
public func PutLiquid(string sznMaterial, int inMaxAmount, object pnSource)
{
	//Wrong material?
	if (sznMaterial != szLiquid)
		if (iVolume>0)
			return 0;
		else if (IsBarrelForMaterial(sznMaterial))
			szLiquid=sznMaterial;
	inMaxAmount = BoundBy(BarrelMaxFillLevel() - iVolume, 0, inMaxAmount);
	iVolume += inMaxAmount;
	return inMaxAmount;
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Rebuy = true;
