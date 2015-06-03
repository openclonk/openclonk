/**
 	Steam Engine 
 	Burns fuels like coal, wood and oil to produce power. The steam engine
 	produces 120 units of power independent of the fuel. However, the fuel 
 	determines the amount of fuel and thereby the burn time.
 	
 	@author Maikel
*/

#include Library_Structure
#include Library_Ownable
#include Library_PowerProducer
#include Library_Flag

local DefaultFlagRadius = 200;

static const SteamEngine_produced_power = 120;

// Variable to store the fuel amount currently held in the engine.
local fuel_amount;

protected func Construction()
{
	fuel_amount = 0;
	return _inherited(...);
}

protected func Initialize()
{
	SetAction("Idle");
	AddTimer("ContentsCheck", 10);
	return _inherited(...);
}

public func IsContainer() { return true; }

protected func RejectCollect(id item, object obj)
{
	if (obj->~IsFuel())
		return false;
	return true;
}

protected func Collection(object obj, bool put)
{
	Sound("Clonk");
}

public func ContentsCheck()
{
	// Ejects non fuel items immediately
	var fuel;
	if(fuel = FindObject(Find_Container(this), Find_Not(Find_Func("IsFuel")))) 
	{
		fuel->Exit(-53, 21, -20, -1, -1, -30);
		Sound("Chuff");
	}
	
	// If active don't do anything.
	if (GetAction() == "Work") 
		return;

	// If there is fuel available let the network know.
	if (fuel_amount > 0 || FindObject(Find_Container(this), Find_Func("IsFuel")))
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
	if (fuel_amount <= 0)
	{
		// Search for new fuel among the contents.
		var fuel = FindObject(Find_Container(this), Find_Func("IsFuel"));
		if (!fuel)
			return false;
		// Extract the fuel amount from the new piece of fuel.	
		fuel_amount += fuel->~GetFuelAmount(true) * 18;
		fuel->RemoveObject();
	}
	// There is enough fuel so start producing power and notify network of this.
	if (GetAction() == "Idle") 
		SetAction("Work");
	return true;
}

// Callback from the power library requesting to stop power production.
public func OnPowerProductionStop(int amount)
{
	// Set action to idle when it was working.
	if (GetAction() == "Work")
		SetAction("Idle");
	return true;
}

// Start call from working action.
protected func WorkStart()
{
	Sound("SteamEngine", false, nil, nil, 1);
	return;
}

// Phase call from working action, every two frames.
protected func Working()
{
	// Reduce the fuel amount by 1 per frame.
	fuel_amount -= 2;
	// Check if there is still enough fuel available.
	if (fuel_amount <= 0)
	{
		// Search for new fuel among the contents.
		var fuel = FindObject(Find_Container(this), Find_Func("IsFuel"));
		if (!fuel)
		{
			// Set action to idle and unregister this producer as available from the network.
			SetAction("Idle");
			UnregisterPowerProduction();
			return;
		}
		// Extract the fuel amount from the new piece of fuel.	
		fuel_amount += fuel->~GetFuelAmount(true) * 18;
		fuel->RemoveObject();
	}
	// Smoke from the exhaust shaft.
	Smoke(-20 * GetCalcDir() + RandomX(-2, 2), -26, 10);
	Smoke(-20 * GetCalcDir() + RandomX(-2, 2), -24, 8);
	Smoke(-20 * GetCalcDir() + RandomX(-2, 2), -24, 10);
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
	Sound("SteamEngine", false, nil, nil, -1);
	return;	
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
}

local ContainBlast = true;
local BlastIncinerate = 130;
local HitPoints = 100;
local Name = "$Name$";
local Description = "$Description$";
