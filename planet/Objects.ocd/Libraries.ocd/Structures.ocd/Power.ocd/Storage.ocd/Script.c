/**
	Power Storage
	Handles some aspects of power storage structures, this library should be
	included by	all power storage structures. Certain functions should be 
	overloaded and others can be used to implement the storage of power in
	a uniform way consistent with the network, see text below.
	
	The power storage has several properties which can be overloaded using the
	following functions:
	 * GetStoragePower() amount of power it consumes or produces.
	 * GetStorageCapacity() its capacity.
	 * GetStorageCoolDown() cool down time between consumption and production.
	Moreover there is a callback for when the stored amount of power changes,
	this can be used to change the appearance of the storage. This callback 
	is OnStoredPowerChange().
	
	If the storage needs be charged from script this is possible via the 
	function SetStoredPower(int to_power).
	
	Important notes when including this library:
	 * The object including this library should return _inherited(...) in the
	   Initialize, Destruction & Definition callback if overloaded.
	
	@author Maikel
*/


// The power storage derives from both the consumer and producer libraries.
#include Library_PowerConsumer
#include Library_PowerProducer


// This object is a power storage: both consumer and producer.
public func IsPowerStorage() { return true; }


/*-- Interface --*/

// This library uses the consumer and producer library interface to the power 
// library. See those scripts for details.


/*-- Callbacks --*/

// Storage power: the amount of power the storage can store or deliver when it
// operatores. 
public func GetStoragePower() { return 0; }

// Storage capacity: the amount of energy a power storage can store. The amount 
// is expressed in power frames.
public func GetStorageCapacity() { return 0; }

// Storage cooldown: number of frames between switches from consumer to producer.
// The default value for this is three seconds.
public func GetStorageCoolDown() { return 36 * 3; }

// Callback by the power storage when the amount of stored power has changed.
public func OnStoredPowerChange()
{
	return _inherited(...);
}


/*-- Library Code --*/

// All power related local variables are stored in a single proplist.
// This reduces the chances of clashing local variables. See 
// Construction for which variables are being used.
local lib_power;

// Set the amount of stored power in this storage. This is limited to be a number
// between zero and what GetStorageCapacity() returns;
public func SetStoredPower(int to_power)
{
	lib_power.stored_power = BoundBy(to_power, 0, GetStorageCapacity());
	// Callback to this object that the power has changed.
	OnStoredPowerChange();
	// Register as consumer and or producer if needed.
	if (!GetEffect("CoolDown", this))
		UpdateNetworkStatus();
	return;
}

// Returns the amount of stored power in the storage.
public func GetStoredPower()
{ 
	return lib_power.stored_power;
}

// Construction callback by the engine: let the power network know this objects
// is available as a storage.
protected func Construction()
{
	// Initialize the single proplist for the power storage library.
	if (lib_power == nil)
		lib_power = {};
	// A single variable to keep track of the stored power in the storage.
	lib_power.stored_power = 0;
	// Now perform the consumer library's initialization.
	_inherited(...);
	// Then set the power need for power storages always to true.
	lib_power.power_need = true;
	return;
}

// Initialize callback by the engine: let the power network know this objects
// is available as a storage.
protected func Initialize()
{
	// Register as a power consumer since it has zero stored energy.
	RegisterPowerRequest(GetStoragePower());
	return _inherited(...);
}

// Destruction callback by the engine: let power network know this object is not
// a storage anymore, it must always be unregistered from the power network.
protected func Destruction()
{
	// Is automatically done by the consumer and producer library.
	return _inherited(...);
}

// Overload the consumer library's no power need function to do nothing.
public func SetNoPowerNeed(bool no_need)
{
	return;
}

// Returns that a storage always has a power need.
public func HasPowerNeed()
{
	return true;
}

// Cooldown effect to prevent continuous switching between consumption and production.
protected func FxCoolDownStart(object target, proplist effect, int temp)
{
	if (temp) 
		return FX_OK;
	// Set Interval to the cool down time.
	effect.Interval = GetStorageCoolDown();	
	return FX_OK;
}

// Cooldown effect to prevent continuous switching between consumption and production.
protected func FxCoolDownTimer(object target, proplist effect, int time)
{
	if (time >= GetStorageCoolDown())
	{
		// After the cool down register the storage as both a producer and consumer if either makes sense.
		UpdateNetworkStatus();
		return FX_Execute_Kill;
	}
	return FX_OK;
}

// Cooldown effect to prevent continuous switching between consumption and production.
protected func FxCoolDownStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return FX_OK;
	return FX_OK;
}

// Updates the network status: registers as consumer and/or producer.
private func UpdateNetworkStatus()
{
	if (lib_power.stored_power > 0)
		RegisterPowerProduction(GetStoragePower());
	if (lib_power.stored_power < GetStorageCapacity())
		RegisterPowerRequest(GetStoragePower());
	return;
}


/*-- Library Code: Production --*/

// Produces power on demand, so not steady.
public func IsSteadyPowerProducer() { return false; }

// Producer priority depends on the amount of power that is stored.
public func GetProducerPriority() { return 50 * (2 * lib_power.stored_power - GetStorageCapacity()) / GetStorageCapacity(); }

// Callback from the power library for production of power request.
public func OnPowerProductionStart(int amount) 
{ 
	// Start the production of power.
	if (!GetEffect("ProducePower", this))
		AddEffect("ProducePower", this, 1, 2, this);
	// Add a cooldown effect to not become consumer at the same time.
	if (!GetEffect("CoolDown", this))
		AddEffect("CoolDown", this, 1, 6, this);
	return true;
}

// Callback from the power library requesting to stop power production.
public func OnPowerProductionStop(int amount)
{
	// Stop the production of power.
	if (GetEffect("ProducePower", this))
		RemoveEffect("ProducePower", this);	
	return true;
}

protected func FxProducePowerStart(object target, proplist effect, int temp)
{
	if (temp) 
		return FX_OK;
	// Set Interval to 2.
	effect.Interval = 2;	
	return FX_OK;
}

protected func FxProducePowerTimer(object target, proplist effect, time)
{
	// Increase the stored power.
	lib_power.stored_power -= effect.Interval * GetStoragePower();
	// Callback to this object that the power has changed.
	OnStoredPowerChange();
	// If stored power is zero then stop producing power.
	if (lib_power.stored_power <= 0)
	{
		// Notify the power network that the storage is empty.
		UnregisterPowerProduction();
		RegisterPowerRequest(GetStoragePower());
		return FX_OK;
	}
	return FX_OK;
}

protected func FxProducePowerStop(object target, proplist effect, int reason, bool temp)
{
	if (temp) 
		return FX_OK;
	return FX_OK;
}


/*-- Library Code: Consumption --*/

// Storage has a low consumer priority so that all other consumers are supplied first.
public func GetConsumerPriority() { return 0; }

// Callback from the power library saying there is enough power.
public func OnEnoughPower(int amount)
{
	// Start the consumption of power.
	if (!GetEffect("ConsumePower", this))
		AddEffect("ConsumePower", this, 1, 2, this);
	// Add a cooldown effect to not become producer at the same time.
	if (!GetEffect("CoolDown", this))
		AddEffect("CoolDown", this, 1, 6, this);
	// Do not return inherited since the no power symbol should not be shown.
	return;
}

// Callback from the power library saying there is not enough power.
public func OnNotEnoughPower(int amount, bool initial_call)
{
	// Stop the consumption of power.
	if (GetEffect("ConsumePower", this))
		RemoveEffect("ConsumePower", this);
	// Do not return inherited since the no power symbol should not be shown.
	return;
}

protected func FxConsumePowerStart(object target, proplist effect, int temp)
{
	if (temp) 
		return FX_OK;
	// Set Interval to 2.
	effect.Interval = 2;	
	return FX_OK;
}

protected func FxConsumePowerTimer(object target, proplist effect, int time)
{
	// Increase the stored power.
	lib_power.stored_power += effect.Interval * GetStoragePower();
	// Callback to this object that the power has changed.
	OnStoredPowerChange();
	// If fully charged remove this effect.
	if (lib_power.stored_power >= GetStorageCapacity())
	{
		// Notify the power network that the storage is full.
		UnregisterPowerRequest();
		RegisterPowerProduction(GetStoragePower());
		return FX_Execute_Kill;
	}
	return FX_OK;
}

protected func FxConsumePowerStop(object target, proplist effect, int reason, bool temp)
{
	if (temp) 
		return FX_OK;
	// Remove a possible cooldown effect as well.
	return FX_OK;
}


/*-- Scenario Saving --*/

public func SaveScenarioObject(proplist props)
{
	if (!inherited(props, ...))
		return false;
	// Save the stored power.	
	if (lib_power.stored_power != nil)
		props->AddCall("StoredPower", this, "SetStoredPower", lib_power.stored_power);
	return true;
}


/*-- Editor Properties --*/

public func Definition(proplist def)
{
	if (!def.EditorProps)
		def.EditorProps = {};
	def.EditorProps.pump_speed = { Name = "$EditorPropStoredPower$", EditorHelp = "$EditorPropStoredPowerHelp$", Type = "enum", Set = "SetStoredPower", Options = [
		{ Value = 0, Name = "$EditorPropStoredPowerEmpty$" },
		{ Value = def->GetStorageCapacity() , Name="$EditorPropStoredPowerFull$" }
	]};
	return _inherited(def, ...);
}

