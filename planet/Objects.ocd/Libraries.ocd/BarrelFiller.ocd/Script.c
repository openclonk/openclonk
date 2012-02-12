/* --- BarrelFiller --- */

/*
Author: ST-DDT
Import this to allow the structures to 
-fill liquids which has been pumped into the building into barrels 
-extract liquids from barrels and pump it somewhere else
*/
 
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
		var ptBarrel = FindObject(Find_Container(this), Find_Func("IsBarrel"), Find_Func("IsBarrelForMaterial", sznMaterial), Find_Not(Find_Func("BarrelIsEmpty")));
		var sztMaterial="";
		if (ptBarrel)
			sztMaterial = ptBarrel->GetBarrelMaterial();
		//Nothing to pump
		if (sztMaterial == "")
			return ["", 0];
		sznMaterial = sztMaterial;
	}
	var itFound = 0;
	for (var ptBarrel in FindObjects(Find_Container(this), Find_Func("IsBarrel"), Find_Func("IsBarrelForMaterial", sznMaterial), Find_Not(Find_Func("BarrelIsEmpty")))) 
	{
		var atFound = ptBarrel->GetLiquid(sznMaterial, inMaxAmount - itFound, this);
		//Crazy stuff happend?
		itFound += BoundBy(atFound[1], 0, inMaxAmount - itFound);
		if (itFound == inMaxAmount)
			break;
	}
	return [sznMaterial, itFound];
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
	var itAmount = inMaxAmount;
	//Fill liquids into already existing barrels
	for (var ptBarrel in FindObjects(Find_Container(this), Find_Func("IsBarrel"), Find_Func("IsContainerForMaterial", sznMaterial), Find_Not(Find_Func("BarrelIsEmpty")), Find_Not(Find_Func("BarrelIsFull")))) 
	{
		itAmount -= BoundBy(ptBarrel->PutLiquid(sznMaterial, itAmount, this), 0, itAmount);
		if (!itAmount)
			return inMaxAmount;
	}
	//Fill liquids into empty barrels
	for (var ptBarrel in FindObjects(Find_Container(this), Find_Func("IsBarrel"), Find_Func("IsContainerForMaterial", sznMaterial), Find_Func("BarrelIsEmpty"))) 
	{
		itAmount -= BoundBy(ptBarrel->PutLiquid(sznMaterial, itAmount, this), 0, itAmount);
		if (!itAmount)
			return inMaxAmount;
	}
	return inMaxAmount - itAmount;
}
