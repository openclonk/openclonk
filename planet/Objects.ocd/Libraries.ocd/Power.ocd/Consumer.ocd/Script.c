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
	The network will then continously search for available power to deliver to
	this consumer and will notify it via the callbacks
	 * OnEnoughPower(int amount)
	 * OnNotEnoughPower()
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
	
	Important notes when including this library:
	 * The object including this library should return _inherited(...) in the
	   Destruction callback if overloaded.
	
	@author Zapper, Maikel
*/


// This object is a power consumer.
public func IsPowerConsumer() { return true; }


/*-- Interface --*/

// Call this function in the power consuming structure to indicate to the network
// a request for power of the specified amount.
private func RegisterPowerRequest(int amount)
{
	Library_Power->RegisterPowerConsumer(this, amount);
	return;
}

// Call this function in the power consuming structure to indicate to the network
// a the end of a power request.
private func UnregisterPowerRequest()
{
	Library_Power->UnregisterPowerConsumer(this);
	return;
}


/*-- Callbacks --*/

// Callback by the power network. Overload this function to start the consumers
// functionality, since enough power is available. return inherited(amount, ...)
// to remove the no-power symbol. It is not allowed to (un)register a power request
// in this callback.
public func OnEnoughPower()
{
	// Remove the no-power symbol.
	RemoveStatusSymbol(Library_PowerConsumer);
	return;
}

// Callback by the power network. Overload this function to stop the consumers
// functionality, since not enough power is available. return inherited(amount, ...)
// to add the no-power symbol. It is not allowed to (un)register a power request
// in this callback.
public func OnNotEnoughPower()
{
	// Show the no-power symbol.
	AddStatusSymbol(Library_PowerConsumer);
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

// Destruction callback by the engine: let power network know this object is not
// a consumer anymore, it must always be unregistered from the power network.
public func Destruction()
{
	UnregisterPowerRequest();
	return _inherited(...);
}
