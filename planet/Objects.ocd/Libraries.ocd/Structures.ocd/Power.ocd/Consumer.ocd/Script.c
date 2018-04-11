/**
	Power Consumer
	Handles some aspects of power consuming structures, this library should be
	included by	all power consuming structures. Certain functions should be 
	overloaded and others can be used to implement the consumption of power in
	a uniform way consistent with the network, see text below.
	
	The interfact is built up such that the consumer can make a request for power
	with the network by using the functions:
	 * RegisterPowerRequest(int amount)
	 * UnregisterPowerRequest()
	 * UpdatePowerRequest() - used internally only.
	The network will then continously search for available power to deliver to
	this consumer and will notify it via the callbacks
	 * OnEnoughPower(int amount)
	 * OnNotEnoughPower(int amount, bool initial_call)
	The last callback will be called when the consumer had enough power and the 
	network stopped delivering to this consumer, due to whatever reason except 
	the consumer itself unregistering the request for power.
	
	The consumer structures can overload the function GetConsumerPriority() and
	return the priority for power requests. The library will deliver power 
	preferentially to consumers with higher priority. In this way certain 
	consumers can be prioritized over others. Typical return values are:
	 * Pump       25
	 * Workshop:  50
	 * Elevator: 100	 
	See also the scripts of these structures for more details on the usage of the
	power consumer library.	
	
	Using the callback GetActualPowerConsumer() the power consumption of an object
	can be passed to its main structure.
	
	The consumer does not consume power if the Rule_NoPowerNeed is active, to set 
	the need for power on a per consumer basis you can use the function 
	SetNoPowerNeed(bool no_need).
	
	Important notes when including this library:
	 * The object including this library should return _inherited(...) in the
	   Initialize and Destruction callback if overloaded.
	
	@author Zapper, Maikel
*/


// This object is a power consumer.
public func IsPowerConsumer() { return true; }


/*-- Interface --*/

// Call this function in the power consuming structure to indicate to the network
// a request for power of the specified amount.
private func RegisterPowerRequest(int amount)
{
	GetPowerSystem()->RegisterPowerConsumer(this, amount);
	return;
}

// Call this function in the power consuming structure to indicate to the network
// a the end of a power request.
private func UnregisterPowerRequest()
{
	GetPowerSystem()->UnregisterPowerConsumer(this);
	// Also ensure that the no-power symbol is not shown any more.
	RemoveStatusSymbol(Library_PowerConsumer);
	return;
}

// Call this function to see if the consumer currently requests power.
private func HasRegisteredPowerRequest()
{
	return GetPowerSystem()->IsRegisteredPowerConsumer(this);
}

// Call this function in the power consuming structure to request and update from
// the power network of this consumer.
private func UpdatePowerRequest()
{
	GetPowerSystem()->UpdateNetworkForPowerLink(this);
	return;
}


/*-- Callbacks --*/

// Callback by the power network. Overload this function to start the consumers
// functionality, since enough power is available. return inherited(amount, ...)
// to remove the no-power symbol. It is not allowed to (un)register a power request
// in this callback.
public func OnEnoughPower(int amount)
{
	// Remove the no-power symbol.
	RemoveStatusSymbol(Library_PowerConsumer);
	return;
}

// Callback by the power network. Overload this function to stop the consumers
// functionality, since not enough power is available. return inherited(amount, ...)
// to add the no-power symbol. It is not allowed to (un)register a power request
// in this callback.
public func OnNotEnoughPower(int amount, bool initial_call)
{
	// Show the no-power symbol.
	ShowStatusSymbol(Library_PowerConsumer);
	return;
}

// Consumer priority: the need of a consumer to have power. This can be used
// by the structures to prioritize certain consumption, for example letting
// an elevator be dominant over a pump.
public func GetConsumerPriority() { return 0; }

// This callback may return an object which acts as the actual power consumer. For example
// the elevator case may return the main elevator object as the main consumer.
public func GetActualPowerConsumer()
{
	return nil;
}


/*-- Library Code --*/

// All power related local variables are stored in a single proplist.
// This reduces the chances of clashing local variables. See 
// Construction for which variables are being used.
local lib_power;

// Construction callback by the engine: check whether the no power need rule is active.
protected func Construction()
{
	// Initialize the single proplist for the power consumer library.
	if (lib_power == nil)
		lib_power = {};
	// A single variable to keep track whether power is needed.
	// Power is not needed when the no power need rule is active.
	lib_power.power_need = !FindObject(Find_ID(Rule_NoPowerNeed));
	return _inherited(...);
}

// Destruction callback by the engine: let power network know this object is not
// a consumer anymore, it must always be unregistered from the power network.
protected func Destruction()
{
	// Only unregister if this object actually is a consumer.
	if (IsPowerConsumer())
		UnregisterPowerRequest();
	return _inherited(...);
}

// When ownership has changed, the consumer may have moved out of or into a new network.
public func OnOwnerChanged(int new_owner, int old_owner)
{
	GetPowerSystem()->TransferPowerLink(this);
	return _inherited(new_owner, old_owner, ...);
}

// By calling this function you can make this consumer ignore the power need.  This is 
// used by the power need rule and can be used by scripters to temporarily turn off the
// need for power in a certain consumer.
public func SetNoPowerNeed(bool no_need)
{
	lib_power.power_need = !no_need;
	// Make sure the power balance of the network is updated.
	UpdatePowerRequest();
	return;
}

// Returns whether this consumer has a power need or not.
public func HasPowerNeed()
{
	return lib_power.power_need;
}
