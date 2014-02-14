/* --- Tank --- */

/*
Author: ST-DDT
Import this to allow the structures to 
-fill liquids which has been pumped into the building into the internal contained 
-extract liquids from internal contained and pump it somewhere else
*/

local szLiquid;
local iLiquidAmount;

public func MaxFillLevel()
{
	return 5000;
}

/**
Extract liquid from this
@param sznMaterial: Material to extract
@param inMaxAmount: Max Amount of Material being extracted 
@param pnPump: Object which extracts the liquid
@param pnPipe: Pipe which extracts the liquid (connected to pnPump)
@param bnWildcard: Usefull to extract random liquids; use '*' for sznMaterial for all Materials
@return [irMaterial,irAmount]
	-irMaterial: Material being extracted
	-irAmount: Amount being extracted
*/
public func LiquidOutput(string sznMaterial, int inMaxAmount, object pnPump, object pnPipe, bool bnWildcard)
{
	//Search liquid to pump
	if (bnWildcard)
	{
		if (WildcardMatch(szLiquid, sznMaterial))
			sznMaterial = szLiquid;
	}
	//Wrong material?
	if (szLiquid != sznMaterial)
		return ["", 0];
	inMaxAmount = Min(inMaxAmount, iLiquidAmount);
	iLiquidAmount -= inMaxAmount;
	return [szLiquid, inMaxAmount];
}

/** 
Insert liquid to this
	@param sznMaterial: Material to insert
	@param inMaxAmount: Max Amount of Material being inserted 
	@param pnPump: Object which inserts the liquid
	@param pnPipe: Pipe which inserts the liquid (connected to pnPump)
	@return irAmount: The inserted amount
*/
public func LiquidInput(string sznMaterial, int inMaxAmount, object pnPump, object pnPipe)
{
	//wrong material?
	if (szLiquid != sznMaterial)
		return 0;
	inMaxAmount = Min(MaxFillLevel() - iLiquidAmount, inMaxAmount);
	iLiquidAmount += inMaxAmount;
	return inMaxAmount;
}

// Set tank liquid type and amount directly
public func SetLiquid(string szNewLiquid, int iNewLiquidAmount)
{
	szLiquid = szNewLiquid;
	iLiquidAmount = iNewLiquidAmount;
	return true;
}


// Scenario saving of liquid fill levels
// Untested. This library is not used. Plus it's called "Libary_Tank" o_O
public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (szLiquid) props->AddCall("Tank", this, "SetLiquid", Format("%v", szLiquid), iLiquidAmount);
	return true;
}
