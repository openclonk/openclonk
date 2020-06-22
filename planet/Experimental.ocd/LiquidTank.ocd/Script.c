/**
	Liquid Tank
	Holds liquids of any type and can be opened to create a flood.
	
	@author Maikel
*/


#include Library_Structure
#include Library_Ownable
#include Library_LiquidContainer
#include Library_PipeControl


public func Initialize()
{
	return _inherited(...);
}

public func Construction(object creator)
{
	return _inherited(creator, ...);
}

public func IsHammerBuildable() { return true; }


/*-- Liquid Control --*/

// Only accept a single liquid at the same time, but accept any liquid type.
public func IsLiquidContainerForMaterial(string liquid)
{
	for (var liquid_content in GetLiquidContents())
		if (GetLiquidDef(liquid) != GetLiquidDef(liquid_content->GetID()))
			return false;
	return true;
}

public func GetLiquidContainerMaxFillLevel(liquid_name)
{
	return this.LiquidCapacity;
}

// Set to source or drain pipe.
public func OnPipeConnect(object pipe, string specific_pipe_state)
{
	if (PIPE_STATE_Source == specific_pipe_state)
	{
		SetSourcePipe(pipe);
		pipe->SetSourcePipe();
	}
	else if (PIPE_STATE_Drain == specific_pipe_state)
	{
		SetDrainPipe(pipe);
		pipe->SetDrainPipe();
	}
	else
	{
		if (!GetDrainPipe())
			OnPipeConnect(pipe, PIPE_STATE_Drain);
		else if (!GetSourcePipe())
			OnPipeConnect(pipe, PIPE_STATE_Source);
	}
	pipe->Report("$MsgConnectedPipe$");
}


/*-- Interaction --*/

public func IsInteractable(object clonk)
{
	if (GetCon() < 100)
		return false;
	return !Hostile(GetOwner(), clonk->GetOwner());
}

public func GetInteractionMetaInfo(object clonk)
{
	if (GetEffect("FxDisperseLiquid", this))
		return { Description = "$MsgCloseTank$", IconName = nil, IconID = Icon_Enter, Selected = false };
	return { Description = "$MsgOpenTank$", IconName = nil, IconID = Icon_Exit, Selected = false };
}

public func Interact(object clonk)
{
	var fx = GetEffect("FxDisperseLiquid", this);
	if (fx)
	{
		fx->Remove();
		return true;
	}
	CreateEffect(FxDisperseLiquid, 100, 2);	
	return true;
}

local FxDisperseLiquid = new Effect
{
	Construction = func()
	{
		this.Interval = 2;
		return FX_OK;
	},
	Timer = func()
	{
		var liquid = Target->Contents();
		if (!liquid || !liquid->~IsLiquid())
			return FX_OK;
		if (liquid->GetLiquidAmount() <= Target.DispersionRate)
		{
			liquid->Exit();
			liquid->SetPosition(Target->GetX(), Target->GetY());
			liquid->Disperse(180, 40);	
		}
		else
		{
			liquid->RemoveLiquid(nil, Target.DispersionRate);
			liquid = liquid->GetID()->CreateLiquid(Target.DispersionRate);
			liquid->SetPosition(Target->GetX(), Target->GetY());
			liquid->Disperse(180, 40);
		}
		// TODO: Sound.
		return FX_OK;
	}
};


/*-- Contents Control --*/

public func IsContainer() { return true; }

protected func RejectCollect(id item, object obj)
{
	// Accept liquids only.
	if (obj->~IsLiquid())
		return _inherited(item, obj, ...);
	return true;
}


/*-- Properties --*/

local Name = "$Name$";
local Description ="$Description$";
local BlastIncinerate = 100;
local FireproofContainer = true;
local HitPoints = 90;
local Components = {Wood = 3, Metal = 2};
local LiquidCapacity = 10000;
local DispersionRate = 40;

// The liquid tank may have one drain and one source.
local PipeLimit_Air = 0;
local PipeLimit_Drain = 1;
local PipeLimit_Source = 1;
