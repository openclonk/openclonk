/**
	Power Consumer
	Handles some aspects of power producing structures, this library 
	should be included by all power producing structures.
	
	Cares about showing the "No Power"-symbol
	and provides CurrentlyHasPower()
	also handles requesting 0 power and the NoPowerNeed-rule correctly
	
	The main part of the power system is handled by Library_Power
	
	Usage:
	a power consumer should always include this library.
	production (or anything else) should /only/ be started or continued in the callback OnEnoughPower.
	in the callback OnNotEnoughPower the user should pause the production.
	
	when everything is ready to produce the user should call MakePowerConsumer(amount) where amount is the amount of power to request (works with 0).
	when the production is done or the building wants to cease consuming power, it should call UnmakePowerConsumer() - note that the callback OnNotEnoughPower is not called this way.

	other:
	the callback GetActualPowerConsumer can return another object that acts as the power consumer when checking for the affiliation to a flag. For example the elevator case could be the power consumer but use the elevator building as the actual power consumer.
	CurrentlyHasPower() returns true when the object has requested power and is not in the sleeping queue.
	
	@author Zapper, Maikel
*/

// Local variables to track power requests and amount.
local has_power = false;
local last_request = 0;
local last_amount = 0;

// States for being able to handle 0-power requests.
static const PowerConsumer_LPR_None = 0;
static const PowerConsumer_LPR_Zero = 1;
static const PowerConsumer_LPR_NonZero = 2;


// This object is a power consumer.
public func IsPowerConsumer() { return true; }

// Consumer priority: the need of a consumer to have power. This can be used
// by the structures to prioritize certain consumption, for example letting
// an elevator be dominant over a pump.
public func GetConsumerPriority() { return 0; }

public func CurrentlyHasPower()
{
	return has_power;
}


/*-- Interface --*/

// Is the object just a part of a building? For example elevator and its case.
public func GetActualPowerConsumer()
{
	return nil;
}

// How much would you like to withdraw your power request?
// Normal objects: not so much, battery: very much.
public func QueryWaivePowerRequest()
{
	return 0;
}

// The object requested power but there is no power!
// Should possibly not use MakePowerConsumer/Producer in this callback.
public func OnNotEnoughPower()
{
	has_power = false;
	// Show symbol.
	this->AddStatusSymbol(Library_PowerConsumer);
	return;
}

// called when the object was deleted from the sleeping queue
// that means, the object had requested power before
public func OnRemovedFromPowerSleepingQueue()
{
	// Remove symbol.
	this->RemoveStatusSymbol(Library_PowerConsumer);
	return;
}

// Called when consumer was sleeping but power is available again.
// Should possibly not use MakePowerConsumer/Producer in this callback.
public func OnEnoughPower()
{
	has_power = true;
	// Remove symbol.
	this->RemoveStatusSymbol(Library_PowerConsumer);
	return;
}

// Add/Remove an effect such that this structure does not need power
public func SetNoPowerNeed(bool to_val)
{
	if (to_val)
		AddEffect("NoPowerNeed", this, 1);
	else
		RemoveEffect("NoPowerNeed", this);
	return true;
}

public func FxNoPowerNeedSaveScen(object obj, proplist fx, proplist props)
{
	// This building doesn't need power, save that to scenario.
	props->AddCall("NoPowerNeed", obj, "SetNoPowerNeed", true);
	return true;
}

// Wrapper for MakePowerConsumer to handle requesting 0 power and the NoPowerNeed rule correctly.
// With an option to just pass on to the global method.
public func MakePowerConsumer(int amount, bool just_pass_to_global)
{	
	if (just_pass_to_global == true)
		return inherited(amount, just_pass_to_global, ...);
	
	var no_power_need = !!ObjectCount(Find_ID(Rule_NoPowerNeed)) || GetEffect("NoPowerNeed", this);
	
	// Don't do anything if the request is the exact same as the previous one (succesive call). 
	if ((amount > 0) && !no_power_need)
		if (last_request == PowerConsumer_LPR_NonZero)
			if (last_amount == amount) 
				return true;
	
	// Special handling for zero amount.
	if ((amount == 0) || no_power_need)
	{
		 // Initially requesting 0 power?
		if (last_request == PowerConsumer_LPR_None)
		{
			last_request = PowerConsumer_LPR_Zero;
			last_amount = amount;
			// Always enable.
			this->~OnEnoughPower();
			return true;
		}
		// Requesting 0 power as a second request.
		else if (last_request == PowerConsumer_LPR_Zero)
		{
			last_amount = amount;
			// Should still have power at this point.
			return true;
		}
		// Requesting 0 power after having requested nonzero power.
		else 
		{
			last_request = PowerConsumer_LPR_Zero;
			last_amount = amount;
			// Remove as official power consumer.
			inherited(0); 
			// Re-enable power supply.
			this->~OnEnoughPower();
			return true;
		}
	}
	else
	{
		// Requesting power != 0.
		last_request = PowerConsumer_LPR_NonZero; 
		last_amount = amount;
	}
	
	return inherited(amount, just_pass_to_global, ...);
}

// Turns the object off as a power consumer.
public func UnmakePowerConsumer()
{
	// Succesive calls have no effect.
	if (last_request == PowerConsumer_LPR_None)
		return true;
		
	// We don't have power anymore.
	has_power = false;
	
	// We were not officially registered as power consumer anyway.
	if (last_request == PowerConsumer_LPR_Zero)
	{
		last_request = PowerConsumer_LPR_None;
		return true;
	}
	last_request = PowerConsumer_LPR_None;
	return MakePowerConsumer(0, true);
}

// Destruction callback: let power network know this object is not a consumer anymore.
public func Destruction()
{
	UnmakePowerConsumer();
	return _inherited(...);
}
