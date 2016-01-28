/**
 * Liquid Container
 *
 * Basic interface for anything that can contain liquids.
 *
 * Author: Ringwaul, ST-DDT, Marky
 */


local lib_liquid_container;

// -------------- Properties
//
// Simple properties that define the object as a liquid container,
// what kind of liquid it can hold, how much it can hold
//
// naming scheme: [verb]LiquidContainer[attribute], because it concerns the container

func IsLiquidContainer() { return true;}

func IsLiquidContainerForMaterial(string liquid_name)
{
	return false;
}

func GetLiquidContainerMaxFillLevel()
{
	return 0;
}

// -------------- Current Status
//
// Simple boolean status checks
//
// naming scheme: LiquidContainer[attribute/question]

func LiquidContainerIsEmpty()
{
	return (GetLiquidFillLevel() == 0);
}

func LiquidContainerIsFull()
{
	return GetLiquidFillLevel() == GetLiquidContainerMaxFillLevel();
}

func LiquidContainerAccepts(string liquid_name)
{
	return IsLiquidContainerForMaterial(liquid_name)
	   && (LiquidContainerIsEmpty() || GetLiquidName() == liquid_name);
}

// -------------- Getters
//
// Getters for stored liquid and amount
// - these should be used primarily by objects that include this library
//
// naming scheme: GetLiquid[attribute], because it concerns the liquid

func GetLiquidName()
{
	//if (LiquidContainerIsEmpty())
	//	return nil; // TODO: was "", this was inconsistent throughout the barrel
	return lib_liquid_container.liquid;
}

func GetLiquidFillLevel()
{
	return lib_liquid_container.volume;
}

// -------------- Setters
//
// Setters for stored liquid and amount
// - these should be used primarily by objects that include this library
//
// naming scheme: SetLiquid[attribute], because it concerns the liquid

func SetLiquidName(string liquid_name)
{
	lib_liquid_container.liquid = liquid_name;
}

func SetLiquidFillLevel(int amount)
{
	ChangeLiquidFillLevel(amount - GetLiquidFillLevel());
}

func ChangeLiquidFillLevel(int amount)
{
	lib_liquid_container.volume += amount;
	
	// Empty the liquid container
	if (LiquidContainerIsEmpty())
	{
		SetLiquidName(nil);
	}
	
	this->UpdateLiquidContainer();
}

// -------------- Interaction
//
// Interfaces for interaction with other objects

/**
Extracts liquid from the container.
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
	liquid_name = liquid_name ?? GetLiquidName();
	amount = amount ?? GetLiquidFillLevel();

	//Wrong material?
	if (!WildcardMatch(GetLiquidName(), liquid_name))
		amount = 0;
	amount = Min(amount, GetLiquidFillLevel());
	ChangeLiquidFillLevel(-amount);
	return [GetLiquidName(), amount];
}

/** 
Inserts liquid into the container.
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

	if (LiquidContainerAccepts(liquid_name))
	{
		SetLiquidName(liquid_name);
		amount = BoundBy(GetLiquidContainerMaxFillLevel() - GetLiquidFillLevel(), 0, amount);
		ChangeLiquidFillLevel(+amount);
		return amount;
	}
	else //Wrong material?
	{
		return 0;
	}
}

// --------------  Internals --------------
//
// Internal stuff

func Construction()
{
	// use proplist to avoid name clashes
	lib_liquid_container = {
		liquid = nil,	// the liquid - this should be a string, so that the container may contain liquids that are not materials
	    volume = 0};	// the stored amount
}

func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (GetLiquidName())
		props->AddCall("Fill", this, "SetLiquidContainer", Format("%v", GetLiquidName()), GetLiquidFillLevel());
	return true;
}

// set the current state, without sanity checks
func SetLiquidContainer(string liquid_name, int amount)
{
	SetLiquidName(liquid_name);
	SetLiquidFillLevel(amount);
}

// interface for updating graphics, etc
func UpdateLiquidContainer()
{
	// do nothing by default
}
