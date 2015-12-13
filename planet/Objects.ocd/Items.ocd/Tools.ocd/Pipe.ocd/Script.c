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
	Sound("Hits::GeneralHit?");
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
	Sound("Objects::Connect");
	
	// If liquid pump has no source yet, create one.
	if (!liquid_pump->GetSource())
	{
		liquid_pump->SetSource(pipe);
		clonk->Message("$MsgCreatedSource$");
		SetGraphics("Source", Pipe, GFX_Overlay, GFXOV_MODE_Picture);
		pipe->SetSource();
	}
	// Otherwise if liquid pump has no drain, create one.
	else
	{
		liquid_pump->SetDrain(pipe);
		clonk->Message("$MsgCreatedDrain$");
		SetGraphics("Drain", Pipe, GFX_Overlay, GFXOV_MODE_Picture);
		pipe->SetDrain();
	}
	return true;
}

// Line broke or something
public func ResetPicture()
{
	SetGraphics("", nil, GFX_Overlay, GFXOV_MODE_Picture);
}

/* Cycling through several aperture offset indices to prevent easy clogging */

// default: pump from bottom vertex
local ApertureOffsetX = 0;
local ApertureOffsetY = 3;

public func CycleApertureOffset()
{
	// Cycle in three steps of three px each through X and Y
	// covering a 3x3 grid on points -3,0,+3
	ApertureOffsetX = (ApertureOffsetX + 6) % 9 - 3;
	if (!ApertureOffsetX) ApertureOffsetY = (ApertureOffsetY + 6) % 9 - 3;
	return true;
}

/* Container dies: Drop connected pipes so they don't draw huge lines over the landscape */

public func IsDroppedOnDeath(object clonk)
{
	return !!FindObject(Find_Func("IsConnectedTo",this));
}
