/*-- Pipe

	Author: ST-DDT
--*/

local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local Collectible = 1;
local Rebuy = true;

protected func Hit()
{
	Sound("GeneralHit?");
}

public func IsToolProduct() { return true; }

/*-- Line connection --*/

/** Will connect power line to building at the clonk's position. */
protected func ControlUse(object clonk, int x, int y)
{
	// Is this already connected to a liquid pump?
	if (FindObject(Find_Func("IsConnectedTo",this)))
		return false;
		
	// Is there an object which accepts power lines?
	var liquid_pump = FindObject(Find_AtPoint(), Find_Func("IsLiquidPump"));
	// No liquid pump, display message.
	if (!liquid_pump)
	{
		clonk->Message("$MsgNoNewPipe$");
		return true;
	}
	
	// already two pipes connected
	if(liquid_pump->GetSource() && liquid_pump->GetDrain())
	{
		clonk->Message("$MsgHasPipes$");
		return true;
	}
	
	// Create and connect pipe.
	var pipe = CreateObjectAbove(PipeLine, 0, 0, NO_OWNER);
	pipe->SetActionTargets(this, liquid_pump);
	Sound("Connect");
	
	// If liquid pump has no source yet, create one.
	if (!liquid_pump->GetSource())
	{
		liquid_pump->SetSource(pipe);
		clonk->Message("$MsgCreatedSource$");
	}
	// Otherwise if liquid pump has no drain, create one.
	else
	{
		liquid_pump->SetDrain(pipe);
		clonk->Message("$MsgCreatedDrain$");
	}
	return true;
}



