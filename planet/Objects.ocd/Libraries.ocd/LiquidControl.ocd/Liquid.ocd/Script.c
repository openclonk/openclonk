/**
 * Liquid
 *
 * Basic interface for liquids. The logic for adding and removing liquid,
 * PutLiquid() and RemoveLiquid() is deliberately the same as in the
 * liquid container, so that scripts can be read more easily.
 *
 * Author: Marky
 */

local liquid;
local volume;

func IsLiquid() { return liquid; } // Default: undefined liquid

// -------------- Callbacks
//
// Engine callbacks

protected func Construction()
{
	liquid = liquid ?? "undefined";
	_inherited(...);
}

func Destruction()
{
	var container = Contained();
	if (container)
	{
		// has an extra slot
		if (container->~HasExtraSlot())
			container->~NotifyHUD();
	}
	return _inherited(...);
}

protected func RejectEntrance(object into)
{
	if (CannotEnter(into)) return true;
	return _inherited(into, ...);
}

func CannotEnter(object into)
{
	// Enters liquid containers only
	if (into->~IsLiquidContainer() || into->~IsLiquidPump())
	{
		for (var liquid in FindObjects(Find_Func("IsLiquid"), Find_Container(into)))
		{
			if (MergeWith(liquid))
			{
				return true; // Cannot enter
			}
		}

		return false; // Enter this object
	}
	else
	{
		return true; // Cannot enter
	}
}

// -------------- Manipulation of liquid amount

func SetLiquidType(string liquid_name)
{
	liquid = liquid_name;
}

func GetLiquidAmount() { return volume; }

func SetLiquidAmount(int amount)
{
	if (amount < 0)
	{
		FatalError(Format("Only positive liquid amounts are allowed, got %d", amount));
	}
	
	volume = amount;

	UpdateLiquidObject();
}

func DoLiquidAmount(int change)
{
	volume += change;
	UpdateLiquidObject();
}

func MergeWith(object liquid_object)
{
	if (WildcardMatch(IsLiquid(), liquid_object->~IsLiquid()))
	{
		liquid_object->DoLiquidAmount(GetLiquidAmount());
		return true;
	}
	return false;
}

// -------------- Status updates
//

func UpdateLiquidObject()
{
	UpdatePicture();
	UpdateMass();
	UpdateName();

	// notify hud
	var container = Contained();
	
	if (volume <= 0)
	{
		RemoveObject();
	}
	
	if (container)
	{
		// has an extra slot
		if (container->~HasExtraSlot())
		{
			container->~NotifyHUD();
		}
		// is a clonk with new inventory system
		else
		{
			container->~OnInventoryChange();
		}
	}
}

func UpdatePicture()
{
	// Allow other objects to adjust their picture.
	return _inherited(...);
}

func UpdateName()
{
	var container = Contained();
	
	if (container && container->~IsLiquidContainer())
	{
		SetName(Format("%d/%d %s", GetLiquidAmount(), container->GetLiquidContainerMaxFillLevel(), GetID()->GetName()));
	}
	else
	{
		SetName(Format("%dx %s", GetLiquidAmount(), GetID()->GetName()));
	}
}

// 1000 liquid items count as 1 mass unit
// this may have to be tuned or made object-specific?
func UpdateMass()
{
	SetMass(GetID()->GetMass() * Max(1, GetLiquidAmount()) / 1000);
}

// 1000 liquid items count as 1 wealth unit
// this may have to be tuned or made object-specific?
func CalcValue(object in_base, int for_plr)
{
	return GetID()->GetValue() * Max(1, GetLiquidAmount()) / 1000;
}

// -------------- Interaction
//
// Interfaces for interaction with other objects


/** 
Inserts liquid into the object.
@param liquid_name: Material to insert
@param amount: Max Amount of Material being inserted 
@param source: Object which inserts the liquid
@return returned_amount: The inserted amount
*/
func PutLiquid(string liquid_name, int amount, object source)
{
	if (amount < 0)
	{
		FatalError(Format("You can insert positive amounts of liquid only, got %d", amount));
	}

	if (IsLiquid() == liquid_name)
	{
		DoLiquidAmount(amount);
		return amount;
	}
	else //Wrong material?
	{
		return 0;
	}
}


/**
Extracts liquid from the object.
@param liquid_name: Material to extract; Wildcardsupport
               Defaults to the current liquid if 'nil' is passed.
@param amount: Max Amount of liquid being extracted;
               Defaults to all contained liquid if 'nil' is passed.
@param destination: Object that extracts the liquid
@return [returned_liquid, returned_amount]
	   - returned_liquid: Material being extracted
	   - returned_amount: Amount being extracted
*/
func RemoveLiquid(string liquid_name, int amount, object destination)
{
	if (amount < 0)
	{
		FatalError(Format("You can remove positive amounts of liquid only, got %d", amount));
	}

	// default parameters if nothing is provided: the current material and level
	liquid_name = liquid_name ?? IsLiquid();
	amount = amount ?? GetLiquidAmount();

	//Wrong material?
	if (!WildcardMatch(IsLiquid(), liquid_name))
		return [IsLiquid(), 0];

	amount = Min(amount, GetLiquidAmount());
	DoLiquidAmount(-amount);
	return [liquid_name, amount];
}


/**
 Converts a liquid name to a definition
 that represents that liquid.
 @par liquid_name the name of the liquid
 @return the Id of the liquid object, 
         or nil if no such object exists
 */
func GetLiquidID(string liquid_name)
{
	if (liquid_name == "Acid") return Liquid_Acid;
	if (liquid_name == "Lava") return Liquid_Lava;
	if (liquid_name == "Oil") return Liquid_Oil;
	if (liquid_name == "Water") return Liquid_Water;
	return Library_Liquid;	
}


/**
 Creates a liquid object with the specified name
 and amount. Liquids with amount 0 can be created
 that way.
 */
func CreateLiquid(string liquid_name, int amount)
{
	var item = CreateObject(GetLiquidID(liquid_name));
	item->SetLiquidType(liquid_name);
	item.volume = amount;
	return item;
}
