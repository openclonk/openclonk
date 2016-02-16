/**
 * Liquid
 *
 * Basic interface for liquids. The logic for adding and removing liquid,
 * PutLiquid() and RemoveLiquid() is deliberately the same as in the
 * liquid container, so that scripts can be read more easily.
 *
 * Author: Marky
 */

#include Library_Stackable


func IsLiquid() { return "undefined"; }
func GetLiquidAmount() { return GetStackCount(); }
func MaxStackCount() { return 999; } // was 1000000, but the stackable_max_count is hindering here. Copying the whole stackable library does not seem useful, though


protected func Construction()
{
	_inherited(...);
	SetStackCount(1); // not max stack!
}


// 10 liquid items count as 1 mass unit
// this may have to be tuned or made object-specific?
private func UpdateMass()
{
	SetMass(GetID()->GetMass() * Max(GetStackCount(), 1) / 10);
}

// 100 liquid items count as 1 wealth unit
// this may have to be tuned or made object-specific?
public func CalcValue(object in_base, int for_plr)
{
	return GetID()->GetValue() * Max(GetStackCount(), 1) / 100;
}


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
		amount = Max(amount, MaxStackCount() - GetStackCount());
		DoStackCount(amount);
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
	DoStackCount(-amount);
	return [liquid_name, amount];
}