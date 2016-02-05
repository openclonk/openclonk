/*-- Pipe

	Author: ST-DDT, Marky
--*/

static const PIPE_STATE_Source = "Source";
static const PIPE_STATE_Drain = "Drain";

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
local PipeState = nil;

protected func Hit()
{
	Sound("Hits::GeneralHit?");
}

public func IsToolProduct() { return true; }

/*-- Line connection --*/

/** Will connect power line to building at the clonk's position. */
protected func ControlUse(object clonk, int x, int y)
{
	// try connecting to a liquid tank first
	if (ConnectPipeToLiquidTank(clonk, nil))
	{
		return true;
	}
	else
	{
		return ConnectPipeToPump(clonk);
	}
}

func CanConnectToLiquidPump()
{
	return PipeState == nil;
}

func CanConnectToLiquidTank(string pipe_state)
{
	if (pipe_state == nil)
	{
		return PipeState != nil;
	}
	else
	{
		return PipeState == pipe_state;
	}
}

func ConnectPipeToPump(object clonk)
{
	// Is there an object which accepts pipes?
	var liquid_pump = FindObject(Find_AtPoint(), Find_Func("IsLiquidPump"));

	// No liquid pump, display message.
	if (!liquid_pump)
	{
		clonk->Message("$MsgNoNewPipe$");
		return true;
	}
	
	// already two pipes connected
	if(liquid_pump->GetSourcePipe() && liquid_pump->GetDrainPipe())
	{
		clonk->Message("$MsgHasPipes$");
		return true;
	}

	if (!ConnectSourcePipeToPump(liquid_pump, clonk)) ConnectDrainPipeToPump(liquid_pump, clonk);
	return true;
}


func ConnectPipeToLiquidTank(object clonk, object tank)
{
	// Is this already connected to a liquid pump?
	var pipe = GetConnectedPipe();
	if (!pipe) return false;

	// Is there an object that accepts pipes?
	if (!tank) tank = FindObject(Find_AtPoint(), Find_Func("IsLiquidTank"));
	if (!tank)
	{
		clonk->Message("$MsgNoNewPipeToTank$");
		return true;
	}
	
	if (PipeState == PIPE_STATE_Source)
	{
		if (tank->QueryConnectSourcePipe(pipe))
		{
			clonk->Message("$MsgHasSourcePipe$");
			return true;
		}
		
		tank->SetSourcePipe(pipe);
	}
	else if (PipeState == PIPE_STATE_Drain)
	{
		if (tank->QueryConnectDrainPipe(pipe))
		{
			clonk->Message("$MsgHasDrainPipe$");
			return true;
		}
		
		tank->SetDrainPipe(pipe);
	}
	else
	{
		FatalError("This code should never be reached");
	}

	pipe->SwitchConnection(this, tank);
	pipe->SetPipeKit(this);
	clonk->Message("$MsgConnectedToTank$", Name, tank->GetName());
	this->OnConnectPipe(tank);
	return true;
}

func GetConnectedPipe()
{
	return FindObject(Find_Func("IsConnectedTo", this));
}

func CreatePipe(object liquid_pump)
{
	// Create and connect pipe.
	var pipe = GetConnectedPipe();
	if (!pipe)
	{
		pipe = CreateObjectAbove(PipeLine, 0, 0, NO_OWNER);
		pipe->SetActionTargets(this, liquid_pump);
	}
	return pipe;
}


func ConnectSourcePipeToPump(object liquid_pump, object clonk)
{
	// If liquid pump has no source yet, create one.
	if (liquid_pump->GetSourcePipe()) return false;
	var pipe = CreatePipe(liquid_pump);
	
	liquid_pump->SetSourcePipe(pipe);
	clonk->Message("$MsgCreatedSource$");
	SetGraphics("Source", Pipe, GFX_Overlay, GFXOV_MODE_Picture);
	Description = "$DescriptionSource$";
	Name = "$NameSource$";
	pipe->SetSource();
	PipeState = PIPE_STATE_Source;

	this->OnConnectPipe(liquid_pump);
	return true;
}

func ConnectDrainPipeToPump(object liquid_pump, object clonk)
{
	// If liquid pump has no drain yet, create one.
	if (liquid_pump->GetDrainPipe()) return false;
	var pipe = CreatePipe(liquid_pump);

	liquid_pump->SetDrainPipe(pipe);
	clonk->Message("$MsgCreatedDrain$");
	SetGraphics("Drain", Pipe, GFX_Overlay, GFXOV_MODE_Picture);
	Description = "$DescriptionDrain$";
	Name = "$NameDrain$";
	pipe->SetDrain();
	PipeState = PIPE_STATE_Drain;
	
	this->OnConnectPipe(liquid_pump);
	return true;
}

func OnConnectPipe(object target)
{
	target->Sound("Objects::Connect");
}

func CutConnection(object target)
{
	var pipe = GetConnectedPipe();
	if (!pipe) return;

	if (pipe->IsConnectedTo(this, true)) // get a strict connection, i.e. connected only to the kit and a structure
	{
		pipe->RemoveObject();
	}
	else if (pipe->IsConnectedTo(target, true)) // we need at least a connection, so that no random guys can cut the pipe
	{
		Exit();
		SetPosition(target->GetX(), target->GetY());
		pipe->SwitchConnection(target, this);
		
		// if we get disconnected from the pump, then we also have to disconnect
		// from all liquid containers: otherwise we would need a logic how to
		// connect from liquid container to pump, which does not exist!
		if (target->~IsLiquidPump())
		{
			CutConnection(this);
		}
	}
	else
	{
		FatalError(Format("Unexpected error: An object %v is trying to cut the pipe connection, but only objects %v and %v may request a disconnect", target, pipe->GetActionTarget(0), pipe->GetActionTarget(1)));
	}
}


// Line broke or something
public func ResetPicture()
{
	SetGraphics("", nil, GFX_Overlay, GFXOV_MODE_Picture);
	Description = "$Description$";
	Name = "$Name$";
	PipeState = nil;
	return true;
}

public func CanBeStackedWith(object other)
{
	// Do not stack source/drain/unused pipes
	return inherited(other) && (PipeState == other.PipeState);
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
