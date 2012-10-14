/**
	Power consumer
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
*/

local PowerConsumer_has_power = false;

// states for being able to handle 0-power requests
static const PowerConsumer_LPR_None = 0;
static const PowerConsumer_LPR_Zero = 1;
static const PowerConsumer_LPR_NonZero = 2;
local PowerConsumer_last_power_request = 0;
local PowerConsumer_last_power_request_amount = 0;


public func IsPowerConsumer() { return true; }

public func CurrentlyHasPower()
{
	return PowerConsumer_has_power;
}

// interface to power handling:

// is the object just a part of a building?
// elevator <-> case for example
public func GetActualPowerConsumer()
{
	return nil;
}

// how much would you like to withdraw your power request?
// normal objects: not so much, battery: very much!
public func QueryWaivePowerRequest()
{
	return 0;
}

// the object requested power but there is no power!
// should possibly not use MakePowerConsumer/Producer in this callback
public func OnNotEnoughPower()
{
	PowerConsumer_has_power = false;
	
	// show symbol
	this->AddStatusSymbol(Library_PowerConsumer);
}

// called when the object was deleted from the sleeping queue
// that means, the object had requested power before
public func OnRemovedFromPowerSleepingQueue()
{
	// remove symbol
	this->RemoveStatusSymbol(Library_PowerConsumer);
}

// called when consumer was sleeping but power is available again
// should possibly not use MakePowerConsumer/Producer in this callback
public func OnEnoughPower()
{
	PowerConsumer_has_power = true;
	
	// remove symbol
	this->RemoveStatusSymbol(Library_PowerConsumer);
}

// Add/Remove an effect such that this structure does not need power
func SetNoPowerNeed(bool to_val)
{
	if (to_val)
		AddEffect("NoPowerNeed", this, 1);
	else
		RemoveEffect("NoPowerNeed", this);
	return true;
}

// wrapper for MakePowerConsumer to handle requesting 0 power and the NoPowerNeed rule correctly
func MakePowerConsumer(int amount, bool just_pass_to_global /* whether to skip special treatment for 0 power request */)
{	
	if(just_pass_to_global == true)
	{
		return inherited(amount, just_pass_to_global, ...);
	}
	
	var no_power_need = !!ObjectCount(Find_ID(Rule_NoPowerNeed)) || GetEffect("NoPowerNeed", this);
	
	if((amount > 0) && !no_power_need) // requesting non-zero?
	if(PowerConsumer_last_power_request == PowerConsumer_LPR_NonZero) // having requested non-zero before?
	if(PowerConsumer_last_power_request_amount == amount) // requesting the exact amount again? (successive call)
		// nothing has changed
		return true;
	
	// special handling for amount == 0
	if((amount == 0) || no_power_need)
	{
		if(PowerConsumer_last_power_request == PowerConsumer_LPR_None) // initially requesting 0 power?
		{
			PowerConsumer_last_power_request = PowerConsumer_LPR_Zero;

			// always enable
			this->~OnEnoughPower();
			return true;
		}
		else if(PowerConsumer_last_power_request == PowerConsumer_LPR_Zero)// requesting 0 power as a second request
		{
			// should still have power at this point
			return true;
		}
		else // requesting 0 power after having requested nonzero power
		{
			PowerConsumer_last_power_request = PowerConsumer_LPR_Zero;
			inherited(0); // removes as official power consumer
			// re-enable power supply
			this->~OnEnoughPower();
			return true;
		}
	}
	else
	{
		PowerConsumer_last_power_request = PowerConsumer_LPR_NonZero; // requesting power != 0
		PowerConsumer_last_power_request_amount = amount;
	}
	
	return inherited(amount, just_pass_to_global,  ...);
}

// turns the object off as a power consumer
func UnmakePowerConsumer()
{
	// succesive calls have no effect
	if(PowerConsumer_last_power_request == PowerConsumer_LPR_None)
		return true;
		
	// we don't have no power anymore
	PowerConsumer_has_power = false;
	
	// we were not officially registered as power consumer anyway
	if(PowerConsumer_last_power_request == PowerConsumer_LPR_Zero)
	{
		PowerConsumer_last_power_request = PowerConsumer_LPR_None;
		return true;
	}
	PowerConsumer_last_power_request = PowerConsumer_LPR_None;
	return MakePowerConsumer(0, true);
}

func Destruction()
{
	UnmakePowerConsumer();
	return _inherited(...);
}