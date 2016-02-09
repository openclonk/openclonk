/*--
	Wooden Barrel
	Author: Ringwaul, ST-DDT

	The barrel is used to transport liquids
--*/

#include Library_CarryHeavy
#include Library_LiquidContainer

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
	AddTimer("Check", 5);
}

private func Hit()
{
	this->PlayBarrelHitSound();
	if (!LiquidContainerIsEmpty())
	{
		if (GBackLiquid(0, this->GetBarrelIntakeY())
		 && GetMaterial(0, this->GetBarrelIntakeY()) != GetLiquidType())
			return;

		EmptyBarrel(GetR());
		Sound("Liquids::Splash1");
	}
}

func GetBarrelIntakeY()
{
	return 3;
}

func PlayBarrelHitSound()
{
	Sound("Hits::Materials::Wood::DullWoodHit?");
}

private func Check()
{
	//Fills Barrel with specified liquid from if submerged
	FillWithLiquid();
	
	//Message("Volume:|%d|Liquid:|%s", iVolume, szLiquid);
}

private func FillWithLiquid()
{
	var intake = this->GetBarrelIntakeY();
	if (!GBackLiquid(0, intake)) return;

	var mat = GetMaterial(0, intake);
	var mat_name = MaterialName(mat);
	if (!LiquidContainerAccepts(mat_name)) return;

	var remaining_volume = GetLiquidContainerMaxFillLevel() - GetLiquidFillLevel();
	var extracted = 0;
	while(extracted < remaining_volume && GetMaterial(0, intake) == mat)
	{
		extracted += 1;
		ExtractLiquid(0, intake);
	}
	
	var inserted = PutLiquid(mat_name, extracted);

	if (inserted < extracted)
	{
		CastPXS(mat_name, extracted - inserted, 1, 0, intake);
	}
}

private func EmptyBarrel(int angle, int strength, object clonk)
{
	if (!angle)
		angle = 0;
	if (!strength)
		strength = 30;
	
	var current_liquid = RemoveLiquid(nil, nil, this);
	var material = current_liquid[0];
	var volume = current_liquid[1];

	CastPXS(material, volume, strength, 0, 0, angle, 30);
	var spray = {};
	spray.Liquid = material;
	spray.Volume = volume;
	spray.Strength = strength;
	spray.Angle = angle;
	spray.Clonk = clonk;
	AddEffect("ExtinguishingSpray", clonk, 100, 1, this, nil, spray);
}

private func UpdateLiquidContainer()
{
	if (LiquidContainerIsEmpty())
	{
		SetColor(RGB(0, 0, 0));
		this.Name = this.Prototype.Name;
		//Value. Base value is 10.
		SetProperty("Value", 10); // TODO: this is a bug! The value is shared by barrel (value:12) and metal barrel (value:16)!
	}
	else
	{
		var liquid_name, color;
		var material = Material(GetLiquidType());
		if (material >= 0)
		{
			var liquid_name = Translate(Format("Material%s", GetLiquidType()));
			var tex = GetMaterialVal("TextureOverlay", "Material", material);
			color = GetAverageTextureColor(tex);
		}
		else
		{
			liquid_name = GetLiquidType();
			color = RGB(0, 0, 0);
		}
		SetColor(color);
		this.Name = Format("%s $NameWith$ %s", this.Prototype.Name, liquid_name);
	}
	return;
}

public func ControlUse(object clonk, int iX, int iY)
{
	var AimAngle = Angle(0, 0, iX, iY);
	if (!LiquidContainerIsEmpty())
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

public func GetLiquidContainerMaxFillLevel()
{
	return 300;
}

public func IsBarrel()
{
	return true;
}

public func IsLiquidContainerForMaterial(string sznMaterial)
{
	return WildcardMatch("Water", sznMaterial) || WildcardMatch("Oil", sznMaterial);
}

public func CanBeStackedWith(object other)
{
	// Does not take into account the fill level for now.
	return inherited(other, ...) && (other->~GetBarrelMaterial() == this->GetBarrelMaterial());
}

public func CalcValue(object in_base, int for_player)
{
	var val = GetDefValue();
	if (!LiquidContainerIsEmpty())
	{
		val += GetValueOf(GetLiquidType()) * GetLiquidFillLevel() / GetLiquidContainerMaxFillLevel();
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


// When is this considered as fuel for the steam engine?
func IsFuel()
{
	return WildcardMatch("Oil", GetLiquidType());
}

// Gets the amount of fuel that is stored in the barrel
func GetFuelAmount(bool partial)
{
	if (partial)
	{
		return SteamEngine->GetFuelValue(GetLiquidType(), GetLiquidFillLevel());
	}

	return SteamEngine->GetFuelValue(GetLiquidType(), GetLiquidContainerMaxFillLevel());
}

// Callback from the steam engine: if this returns true, then the barrel is not removed
func OnFuelRemoved(int amount)
{
	RemoveLiquid(nil, amount);
	return true;
}



public func Definition(proplist def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(0, 1000, 0), Trans_Rotate(-40, 1, 0, 0), Trans_Rotate(20, 0, 0, 1)), def);
}

local Collectible = true;
local Name = "$Name$";
local Description = "$Description$";
local ContactIncinerate = 2;