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

func CollectFromStack(object item)
{
	// Callback from stackable object: Try grabbing partial objects from this stack, if the stack is too large
	if (item->GetStackCount() > GetLiquidAmountRemaining() && !this->RejectStack(item))
	{
		// Get one sample object and try to insert it into the barrel
		var candidate = item->TakeObject();
		candidate->Enter(this);
		
		// Put it back if it was not collected
		if (candidate && !(candidate->Contained()))
		{
			item->TryAddToStack(candidate);
		}
	}
}

func RejectStack(object item)
{
	// Callback from stackable object: When should a stack entrance be rejected, if the object was not merged into the existing stacks?
	if (Contents())
	{
		// The barrel can hold only one type of liquid
		return true;
	}
	if (item->~IsLiquid() && this->~IsLiquidContainerForMaterial(item->~GetLiquidType()))
	{
		// The liquid is suitable, collect it!
		return false;
	}
	else
	{
		// Reject anything else
		return true;
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
	if (GetLiquidAmount() >= GetLiquidContainerMaxFillLevel()) return;
	
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
	
	var inserted = 0;
	if (extracted > 0) inserted = PutLiquid(mat_name, extracted);

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

		UpdateLiquidContainer();
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
	return !!WildcardMatch("Water", liquid_name) || !!WildcardMatch("Oil", liquid_name);
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
		var name = Format("%s $NameWith$ %s", this.Prototype.Name, Contents().Prototype.Name);
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


// Sells the contents only, leaving an empty barrel.
// Empty barrels can then be sold separately.
public func QueryOnSell(int for_player, object in_base)
{
	if (Contents() && in_base)
	{
		// Sell contents first
		for(var contents in FindObjects(Find_Container(this)))
		{
			in_base->~DoSell(contents, for_player);
		}
		return true;
	}
	return false;
}

local Collectible = true;
local Name = "$Name$";
local Description = "$Description$";
local ContactIncinerate = 2;
local BarrelIntakeY = 3;
