/*--
	Wooden Barrel
	Author: Ringwaul, ST-DDT

	The barrel is used to transport liquids
--*/

#include Library_CarryHeavy
#include Library_LiquidContainer
#include Library_HasExtraSlot

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

func RejectCollect(id def, object item)
{
	Log("***** Barrel: Called reject collect");
	if (Contents() && def != Contents()->GetID())
	{
		Log("***** Barrel: Reject collection because contents");
		return true;
	}
	if (item && item->~IsLiquid() && this->~IsLiquidContainerForMaterial(item->~GetLiquidType()))
	{
		return false; // Collect it!
	}
	else
	{
		return true; // Reject it!
	}
}

private func Hit()
{
	this->PlayBarrelHitSound();
	if (Contents())
	{
		if (GBackLiquid(0, this.BarrelIntakeY)
		 && GetMaterial(0, this.BarrelIntakeY) != Contents()->GetLiquidType())
			return;

		EmptyBarrel(GetR());
		Sound("Liquids::Splash1");
	}
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
	var intake = this.BarrelIntakeY;
	if (!GBackLiquid(0, intake)) return;

	var mat = GetMaterial(0, intake);
	var mat_name = MaterialName(mat);
	if (!IsLiquidContainerForMaterial(mat_name)) return;

	var remaining_volume = GetLiquidContainerMaxFillLevel() - GetLiquidAmount();
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
	if (Contents())
	{
		var material = Contents()->~GetLiquidType();
		var volume = Contents()->~GetLiquidAmount();
	
		Contents()->~Disperse(angle, strength);

		var spray = {};
		spray.Liquid = material;
		spray.Volume = volume;
		spray.Strength = strength;
		spray.Angle = angle;
		spray.Clonk = clonk;
		AddEffect("ExtinguishingSpray", clonk, 100, 1, this, nil, spray);
	}
}

private func UpdateLiquidContainer()
{
	if (Contents())
	{
		var color;
		var material = Material(Contents()->GetLiquidType());
		if (material >= 0)
		{
			var tex = GetMaterialVal("TextureOverlay", "Material", material);
			color = GetAverageTextureColor(tex);
		}
		else
		{
			color = RGB(0, 0, 0);
		}
		SetColor(color);
	}
	else
	{
		SetColor(RGB(0, 0, 0));
		//Value. Base value is 10.
		SetProperty("Value", 10); // TODO: this is a bug! The value is shared by barrel (value:12) and metal barrel (value:16)!
	}

	this.Name = GetNameForBarrel();
	return;
}

public func ControlUse(object clonk, int iX, int iY)
{
	var AimAngle = Angle(0, 0, iX, iY);
	if (Contents())
	{
		EmptyBarrel(AimAngle, 50, clonk);
		if (iX > 1)
			Contained()->SetDir(1);
		if (iX < -1)
			Contained()->SetDir(0);
	}
	return true;
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

public func IsLiquidContainerForMaterial(string liquid_name)
{
	return WildcardMatch("Water", liquid_name) || WildcardMatch("Oil", liquid_name);
}

public func CanBeStackedWith(object other)
{
	// Does not take into account the fill level for now.
	var liquid = other->Contents();
	var my_liquid = this->Contents();
	var both_filled = (my_liquid != nil) && (liquid != nil);
	var both_empty = !my_liquid && !liquid;

	if (both_filled) both_filled = (liquid->~GetLiquidType() == Contents()->~GetLiquidType());
	
	return _inherited(other, ...) && (both_empty || both_filled);
}


func GetNameForBarrel()
{
	if (Contents())
	{
		var name = Format("%s $NameWith$ %s", this.Prototype.Name, Contents()->GetName());
		return name;
	}
	else
	{
		return this.Prototype.Name;
	}
}


public func Definition(proplist def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(0, 1000, 0), Trans_Rotate(-40, 1, 0, 0), Trans_Rotate(20, 0, 0, 1)), def);
}

func Collection2(object item)
{
	UpdateLiquidContainer();
	return _inherited(item, ...);
}

func Ejection(object item)
{
	UpdateLiquidContainer();
	return _inherited(item, ...);
}

local Collectible = true;
local Name = "$Name$";
local Description = "$Description$";
local ContactIncinerate = 2;
local BarrelIntakeY = 3;
