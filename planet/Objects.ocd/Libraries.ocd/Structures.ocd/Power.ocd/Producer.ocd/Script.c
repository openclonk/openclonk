/**
	Power Producer
	Handles some aspects of power producing structures, this library should be 
	included by all power producing structures. Certain functions should be 
	overloaded and others can be used to implement the production of power
	in a uniform way consistent with the network, see text below.
	 
	The interface is built up such that the producer can let the network know it
	is ready to deliver power to the network by the functions 
	 * RegisterPowerProduction(int amount)
	 * UnregisterPowerProduction()
	Then the network will address the power producer via the callbacks
	 * OnPowerProductionStart(int amount)
	 * OnPowerProductionStop()
	to start and stop the power production. The producer should overload these 
	functions and return true if the starting or stopping has succeeded. For
	steady producers these calls are not made and a steady production of power 
	is assumed. A steady producer should have return value true for the function
	 * IsSteadyPowerProducer()
	 
	These structures can overload the function GetProducerPriority() and return 
	the priority for delivering power to the network. The library will 
	preferentially drain power from the producers with highest priority, so that
	on-demand producers are not drained	before power storages or steady producers.
	Typical return values are:
	 * Wind generator:  100
	 * Compensator:      50
	 * Steam engine:      0	 
	See also the scripts of these structures for more details on the usage of the
	power producer library.
	
	Important notes when including this library:
	 * The object including this library should return _inherited(...) in 
	   the Destruction callback if overloaded.
	 * If the power producer also serves as a flag it is cleaner to initialize 
	   the flag before making any power requests.
	
	@author Zapper, Maikel
*/


// This object is a power producer.
public func IsPowerProducer() { return true; }


/*-- Interface -- */

// Call this function in the power producing structure to indicate to the network
// that this structure is available and able to produce the specified amount of power.
private func RegisterPowerProduction(int amount)
{
	GetPowerSystem()->RegisterPowerProducer(this, amount);
	return;
}

// Call this function in the power producing structure to indicate to the network
// that this structure is not able to produce any power any more.
private func UnregisterPowerProduction()
{
	GetPowerSystem()->UnregisterPowerProducer(this);
	return;
}

// Call this function to see if the producer currently has registere power production.
private func HasRegisteredPowerProduction()
{
	return GetPowerSystem()->IsRegisteredPowerProducer(this);
}


/*-- Callbacks --*/
	
// Callback by the power network. Overload this function and start the production 
// of power in this structure for the requested amount if possible. 
public func OnPowerProductionStart(int amount) 
{ 
	// A return value of false indicates to the network that power production is not possible.
	return false;
}

// Callback by the power network. Overload this function and stop the production
// of power in this structure if possible.
public func OnPowerProductionStop(int amount)
{
	// A return value of false indicates to the network that stopping the current production
	// was not possible, this should in principle never happen. However, if so the network
	// must assume this producer is still actively delivering power.
	return true;
}

// Whether this object is a steady power producer which continuously delivers
// power to the network it is in, a producer is not steady by default.
public func IsSteadyPowerProducer() { return false; }

// Producer priority: the willingsness of a producer to deliver energy.
// This is high for steady producers like the wind generator and low
// for producers like the steam engine. 
public func GetProducerPriority() { return 0; }


/*-- Library Code --*/

// Destruction callback by the engine: let power network know this object is not a producer 
// anymore. A power producer must always be unregistered from the network.
protected func Destruction()
{
	UnregisterPowerProduction();
	return _inherited(...);
}

// When ownership has changed, the producer may have moved out of or into a new network.
public func OnOwnerChanged(int new_owner, int old_owner)
{
	GetPowerSystem()->TransferPowerLink(this);
	return _inherited(new_owner, old_owner, ...);
}
