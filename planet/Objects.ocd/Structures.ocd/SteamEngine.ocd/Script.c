/**
 	Steam Engine 
 	Burns fuels like coal, wood and oil to produce power. The steam engine
 	produces 120 units of power independent of the fuel. However, the fuel 
 	determines the amount of fuel and thereby the burn time.
 	
 	@author Maikel (orignal script), Marky (fuel liquid)
*/

#include Library_Structure
#include Library_Ownable
#include Library_PowerProducer
#include Library_Flag
#include Library_Tank

local DefaultFlagRadius = 200;

static const SteamEngine_produced_power = 120;

local fuel_amount;

protected func Initialize()
{
	fuel_amount = 0;
	SetAction("Idle");
	AddTimer("ContentsCheck", 10);
	return _inherited(...);
}

public func IsHammerBuildable() { return true; }

public func IsContainer() { return true; }

protected func RejectCollect(id item, object obj)
{
	// Accept fuel only
	if (obj->~IsFuel())
		return false;

	// Is the object a container? If so, try to empty it.
	if (obj->~IsContainer() || obj->~IsLiquidContainer())
	{
		GrabContents(obj);
	}
	return true;
}

protected func Collection(object obj, bool put)
{
	Sound("Objects::Clonk");
}

public func ContentsCheck()
{
	// Ejects non fuel items immediately
	var fuel;
	if (fuel = FindObject(Find_Container(this), Find_Not(Find_Func("IsFuel"))))
	{
		fuel->Exit(-45, 21, -20, -1, -1, -30);
		Sound("Chuff");
	}
	
	// If active don't do anything.
	if (IsWorking()) 
		return;

	// If there is fuel available let the network know.
	if (GetFuelAmount() > 0 || GetFuelContents())
		RegisterPowerProduction(SteamEngine_produced_power);
	return;
}


public func GetFuelAmount()
{
	return fuel_amount;
}


/*-- Power Production --*/

// Produces power on demand, so not steady.
public func IsSteadyPowerProducer() { return false; }

// Low priority so that other sources of power are drained before burning fuel.
public func GetProducerPriority() { return 0; }

// Callback from the power library for production of power request.
public func OnPowerProductionStart(int amount) 
{ 
	// Check if there is fuel.
	RefillFuel();
	// There is enough fuel so start producing power and notify network of this.
	if (GetAction() == "Idle") 
		SetAction("Work");
	return true;
}

// Callback from the power library requesting to stop power production.
public func OnPowerProductionStop(int amount)
{
	// Set action to idle when it was working.
	if (IsWorking())
		SetAction("Idle");
	return true;
}

// Start call from working action.
protected func WorkStart()
{
	Sound("Structures::SteamEngine", {loop_count = 1});
	return;
}

// Status?
protected func IsWorking(){ return GetAction() == "Work";}

// Phase call from working action, every two frames.
protected func Working()
{
	DoFuelAmount(-2); // Reduce the fuel amount by 1 per frame
	RefillFuel(); // Check if there is still enough fuel available.

	if (!GetFuelAmount())
	{
		// Set action to idle and unregister this producer as available from the network.
		SetAction("Idle");
		UnregisterPowerProduction();
	}
	Smoking(); // Smoke from the exhaust shaft.
	return;
}

// Stop call from working action.
protected func WorkStop()
{
	// Don't kill the sound in this call, since that would interupt the sound effect.
	return;
}

// Abort call from working action.
protected func WorkAbort()
{
	// Sound can be safely stopped here since this action will always end with an abort call.
	Sound("Structures::SteamEngine", {loop_count = -1});
	return;	
}

func RefillFuel()
{
	// Check if there is still enough fuel available.
	var no_fuel = GetFuelAmount() <= 0;
	// The reserve is probably not necessary
	var should_keep_reserve = IsWorking() && GetNeutralPipe() && GetFuelAmount() < 100;
	if (no_fuel || should_keep_reserve)
	{
		var fuel_extracted;
	
		// Search for new fuel among the contents.
		var fuel = GetFuelContents();
		if (fuel)
		{
			fuel_extracted = fuel->~GetFuelAmount();
			if (!fuel->~OnFuelRemoved(fuel_extracted)) fuel->RemoveObject();
	
			DoFuelAmount(fuel_extracted * 18);
		}
	}
}

func GetFuelContents()
{
	return FindObject(Find_Container(this), Find_Func("IsFuel"));
}

func DoFuelAmount(int amount)
{
	fuel_amount += amount;
}

func Smoking()
{
	// Smoke from the exhaust shaft
	Smoke(-20 * GetCalcDir() + RandomX(-2, 2), -26, 10);
	Smoke(-20 * GetCalcDir() + RandomX(-2, 2), -24, 8);
	Smoke(-20 * GetCalcDir() + RandomX(-2, 2), -24, 10);
}


public func IsLiquidContainerForMaterial(string liquid)
{
	return WildcardMatch("Oil", liquid);
}

public func GetLiquidContainerMaxFillLevel(liquid_name)
{
	return 300;
}

// The foundry may have one drain and one source.
public func QueryConnectPipe(object pipe)
{
	if (GetDrainPipe() && GetSourcePipe())
	{
		pipe->Report("$MsgHasPipes$");
		return true;
	}
	else if (GetSourcePipe() && pipe->IsSourcePipe())
	{
		pipe->Report("$MsgSourcePipeProhibited$");
		return true;
	}
	else if (GetDrainPipe() && pipe->IsDrainPipe())
	{
		pipe->Report("$MsgDrainPipeProhibited$");
		return true;
	}
	else if (pipe->IsAirPipe())
	{
		pipe->Report("$MsgPipeProhibited$");
		return true;
	}
	return false;
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


/*-- Properties --*/

local ActMap = {
	Idle = {
		Prototype = Action,
		Name = "Idle",
		Procedure = DFA_NONE,
		Directions = 2,
		FlipDir = 1,
		Length = 1,
		Delay = 0,
		FacetBase = 1,
		NextAction = "Idle",
	},
	Work = {
		Prototype = Action,
		Name = "Work",
		Procedure = DFA_NONE,
		Directions = 2,
		FlipDir = 1,
		Length = 20,
		Delay = 2,
		FacetBase = 1,
		NextAction = "Work",
		Animation = "Work",
		PhaseCall = "Working",
		StartCall = "WorkStart",
		EndCall = "WorkStop",
		AbortCall = "WorkAbort",
	},
};

protected func Definition(def) 
{
	SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(25, 0, 1, 0), Trans_Scale(625)), def);
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(-4000, -18000, 60000), Trans_Rotate(25, 0, 1, 0), Trans_Scale(625)), def);
	return _inherited(def, ...);
}

local ContainBlast = true;
local BlastIncinerate = 130;
local HitPoints = 100;
local FireproofContainer = true;
local Name = "$Name$";
local Description = "$Description$";
local Components = {Rock = 6, Metal = 3};