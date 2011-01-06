/*-- Pipe --*/

protected func Hit()
{
	Sound("RockHit*");
}

public func IsToolProduct() { return 1; }

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

local Name = "$Name$";
local Description = "$Description$";
