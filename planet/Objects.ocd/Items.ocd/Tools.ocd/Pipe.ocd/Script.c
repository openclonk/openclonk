/*-- Pipe --*/

//Author: ST-DDT

protected func Hit()
{
	Sound("GeneralHit?");
}

public func IsToolProduct() { return true; }

/*-- Line connection --*/

// Called with double dig: will connect power line to building at the clonk's position.
protected func ControlUse(object clonk, int x, int y)
{
	// Is this already connected to a liquid pump?
	if (FindObject(Find_PipeLine()))
		return false;
	// Only use if clonk is walking.
	if (!clonk->IsWalking())
		return true;
	// Clonk should stand still.
	clonk->SetComDir(COMD_Stop);
	// Is there an object which accepts power lines?
	var liquid_pump = FindObject(Find_AtPoint(), Find_Func("IsLiquidPump"));
	// No liquid pump, display message.
	if (!liquid_pump)
	{
		clonk->Message("$MsgNoNewPipe$");
		return true;
	}
	// Create and connect pipe.
	var pipe;
	// If liquid pump has no source yet, create one.
	if (!liquid_pump->GetSource())
	{
		pipe = CreateObject(PipeLine, 0, 0, NO_OWNER);
		pipe->SetActionTargets(this, liquid_pump);
		liquid_pump->SetSource(pipe);
		Sound("Connect");
		clonk->Message("$MsgCreatedSource$");
		return true;
	}
	// Otherwise if liquid pump has no drain, create one.
	if (!liquid_pump->GetDrain())
	{
		pipe = CreateObject(PipeLine, 0, 0, NO_OWNER);
		pipe->SetActionTargets(this, liquid_pump);
		liquid_pump->SetDrain(pipe);
		Sound("Connect");
		clonk->Message("$MsgCreatedDrain$");
		return true;
	}
	// Otherwise do nothing and notify player.
	clonk->Message("MsgHasPipes");
	return true;
}

// Finds all pipe lines connected to obj (can be nil in local calls).
private func Find_PipeLine(object obj)
{
	if (!obj)
		obj = this;
	return [C4FO_Func, "IsConnectedTo", obj];
}

/**
Extract liquid from this
@param sznMaterial: Material to extract. 0 or "*" for any material.
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
	var itMaterial;
	//Search liquid to pump
	if (bnWildcard)
	{
		itMaterial = GetMaterial();
		//nothing?
		if (itMaterial == -1)
			return ["", 0];
		//no liquid?
		if (GetMaterialVal("Density", "Material", itMaterial) != 25)
			return ["", 0];
		//wrong liquid?
		if (sznMaterial)
			if (!WildcardMatch(MaterialName(itMaterial),sznMaterial))
				return ["", 0];
		sznMaterial = MaterialName(itMaterial);
	}
	else
		itMaterial = Material(sznMaterial);
	if (GetMaterial() != itMaterial)
		return ["", 0];
	var itFound = ExtractMaterialAmount(0, 0, itMaterial, 5);
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
	var i=Max(0,inMaxAmount),itMaterial=Material(sznMaterial);
	while (i--) InsertMaterial(itMaterial);
	return inMaxAmount;
}

local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local Collectible = 1;
local Rebuy = true;
