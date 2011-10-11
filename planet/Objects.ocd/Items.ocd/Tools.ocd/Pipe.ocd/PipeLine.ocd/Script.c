/*-- Pipe line --*/

//Author: ST-DDT

protected func Initialize()
{
	SetAction("Connect");
	SetVertexXY(0, GetX(), GetY());
	SetVertexXY(1, GetX(), GetY());
	SetProperty("LineColors", [RGB(80, 80, 120), RGB(80, 80, 120)]);
	return;
}

// Returns true if this object is a functioning pipe.
public func IsPipeLine()
{
	return GetAction() == "Connect";
}

// Returns whether this pipe is connected to an object.
public func IsConnectedTo(object obj)
{
	return GetActionTarget(0) == obj || GetActionTarget(1) == obj;
}

// Returns the object which is connected to obj through this pipe.
public func GetConnectedObject(object obj)
{
	if (GetActionTarget(0) == obj)
		return GetActionTarget(1);
	if (GetActionTarget(1) == obj)
		return GetActionTarget(0);
	return;
}

protected func LineBreak(bool no_msg)
{
	Sound("LineBreak");
	if (!no_msg)
		BreakMessage();
	return;
}

private func BreakMessage()
{
	var line_end = GetActionTarget(0);
	if (line_end->GetID() != Pipe)
		line_end = GetActionTarget(1);
	
	line_end->Message("$TxtPipeBroke$");
	return;
}

local ActMap = {
	Connect = {
		Prototype = Action,
		Name = "Connect",
		Procedure = DFA_CONNECT,
		NextAction = "Connect"
	}
};

/**
Extract liquid from barrel
@param sznMaterial: Material to extract; Wildcardsupport
@param inMaxAmount: Max Amount of Material being extracted 
@param pnTarget: Object which extracts the liquid
@return [irMaterial,irAmount]
	-irMaterial: Material being extracted
	-irAmount: Amount being extracted
*/
public func GetLiquid(string sznMaterial, int inMaxAmount, object pnTarget, bool bWildcard)
{
	var pConnected = GetConnectedObject(pnTarget);
	if (!pConnected)
		return ["", 0];
	var aMat = pConnected->~LiquidOutput(sznMaterial, inMaxAmount, pnTarget, this, bWildcard);
	//Bad script? Not needed.
	if (GetType(aMat) != C4V_Array)
		return [-1, 0];
	//Verify data
	if ((aMat[0] == "") || (GetLength(aMat) == 1))
		aMat[1] = 0;
	//Nothing is nothing
	if (aMat[1] <= 0)
	{
		aMat[0] = "";
		aMat[1] = 0;
	} //Bad script end
	return aMat;
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
	var pConnected = GetConnectedObject(pnSource);
	if (!pConnected)
		return 0;
	if (sznMaterial == "")
		return 0;
	return BoundBy(pConnected->~LiquidInput(sznMaterial, inMaxAmount, pnSource, this), 0, inMaxAmount);
}

local Name = "$Name$";
		
