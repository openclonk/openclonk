/*--
	Wooden Barrel
	Author: Ringwaul, ST-DDT

	The barrel is used to transport liquids
--*/

#include Library_CarryHeavy

local szLiquid;
local iVolume;

public func GetCarryTransform(clonk)
{
	if(GetCarrySpecial(clonk))
		return Trans_Translate(1000, 6500, 0);
	
	return Trans_Translate(1500, 0, -1500);
}
public func GetCarryPhase()
{
	return 900;
}

protected func Initialize()
{
	iVolume = 0;
	AddTimer("Check", 5);
}

private func Hit()
{
	Sound("Hits::Materials::Wood::DullWoodHit?");
	if (iVolume >= 1)
	{
		if (GBackLiquid(0, 3) && GetMaterial(0, 3) != szLiquid)
			return 0;
		EmptyBarrel(GetR());
		Sound("Liquids::Splash1");
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
		SetColor(RGB(0,0,0));
		szLiquid = nil;
	}
	
	//Value. Base value is 10.
	if (iVolume == 0)
		SetProperty("Value", 10);
	
	//Message("Volume:|%d|Liquid:|%s", iVolume, szLiquid);
}

private func FillWithLiquid()
{
	var mat = GetMaterial();
	if (AcceptMaterial(mat))
	{
		FillBarrel(MaterialName(mat));
		UpdateBarrel();
	}
}

private func AcceptMaterial(int material)
{
	// Accepts only water.
	return material == Material("Water");
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

private func EmptyBarrel(int angle, int strength, object clonk)
{
	if (!angle)
		angle = 0;
	if (!strength)
		strength = 30;
	CastPXS(szLiquid, iVolume, strength, 0, 0, angle, 30);
	var spray = {};
	spray.Liquid = szLiquid;
	spray.Volume = iVolume;
	spray.Strength = strength;
	spray.Angle = angle;
	spray.Clonk = clonk;
	AddEffect("ExtinguishingSpray", clonk, 100, 1, this, nil, spray);
	iVolume = 0;
	UpdateBarrel();
}

private func UpdateBarrel()
{
	if (iVolume == 0)
	{
		SetColor(RGB(0,0,0));
		this.Name = this.Prototype.Name;
	}
	else
	{
		var tex = GetMaterialVal("TextureOverlay","Material",Material(szLiquid));
		var color = GetAverageTextureColor(tex);
		SetColor(color);
		var materialTranslation = Translate(Format("Material%s",szLiquid));
		this.Name = Format("%s $NameWith$ %s", this.Prototype.Name, materialTranslation);
	}
	return;
}

public func ControlUse(object clonk, int iX, int iY)
{
	var AimAngle = Angle(0, 0, iX, iY);
	if (iVolume >= 1)
	{
		EmptyBarrel(AimAngle, 50, clonk);
		if (iX > 1)
			Contained()->SetDir(1);
		if (iX < -1)
			Contained()->SetDir(0);
	}
	return 1;
}

protected func FxExtinguishingSprayStart(object target, proplist effect, int temp, proplist spray)
{
	if (temp)
		return FX_OK;
	// Only for extinguishing materials.	
	if (!GetMaterialVal("Extinguisher", "Material",	Material(spray.Liquid)))
		return FX_Start_Deny;		
	// If used by an object also extinguish that.
	if (spray.Clonk)
		spray.Clonk->Extinguish(Min(100, spray.Volume/3));
	// Store spray parameters.	
	effect.Volume = spray.Volume;
	effect.Strength = spray.Strength;
	effect.Angle = spray.Angle;
	return FX_OK;
}

protected func FxExtinguishingSprayTimer(object target, proplist effect, int time)
{
	// Move three lines from the barrel outwards along the defined angle.
	// And extinguish all objects on these lines.
	if (time > 20)
		return FX_Execute_Kill;
	var d = effect.Strength * time / 25;
	for (var dev = -10; dev <= 10; dev+= 10)
	{ 
		var x = Sin(effect.Angle + dev, d);
		var y = -Cos(effect.Angle + dev, d);
		if (PathFree(GetX(), GetY(), GetX() + x, GetY() + y))
			for (var obj in FindObjects(Find_AtPoint(x, y), Find_OCF(OCF_OnFire)))
				obj->Extinguish(Max(0, effect.Volume/3 - 2 * d));
	}
	return FX_OK;
}

public func IsToolProduct() { return true; }

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

public func IsLiquidContainer() { return true; }

public func CanBeStackedWith(object other)
{
	// Does not take into account the fill level for now.
	return inherited(other, ...) && (other->~GetBarrelMaterial() == this->GetBarrelMaterial());
}

public func SetFilled(material, volume)
{
	szLiquid = material;
	iVolume = volume;
	UpdateBarrel();
}

public func CalcValue(object in_base, int for_player)
{
	var val = GetDefValue();
	if (iVolume > 0)
	{
		val += GetValueOf(szLiquid) * iVolume / 300;
	}
	return val;
}

private func GetValueOf(string szMaterial) // 300 px of...
{
	// just some provisional values, feel free to change them
	// for gameplay reasons
	if (szMaterial == "Water") return -6;
	if (szMaterial == "Lava") return -10;
	if (szMaterial == "DuroLava") return -10;
	if (szMaterial == "Acid") return -8;
	if (szMaterial == "Firefluid") return 10;
	return 0;
}

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (szLiquid) props->AddCall("Fill", this, "SetFilled", Format("%v", szLiquid), iVolume);
	return true;
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
	UpdateBarrel();
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
		if (iVolume > 0)
			return 0;
		else if (IsBarrelForMaterial(sznMaterial))
			szLiquid = sznMaterial;
	inMaxAmount = BoundBy(BarrelMaxFillLevel() - iVolume, 0, inMaxAmount);
	iVolume += inMaxAmount;
	UpdateBarrel();
	return inMaxAmount;
}

public func Definition(proplist def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(0, 1000, 0), Trans_Rotate(-40, 1, 0, 0), Trans_Rotate(20, 0, 0, 1)), def);
}

local Collectible = true;
local Name = "$Name$";
local Description = "$Description$";
local ContactIncinerate = 2;