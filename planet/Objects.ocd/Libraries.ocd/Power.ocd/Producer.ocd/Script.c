/**
	Power Producer
	Handles some aspects of power producing structures, this library should be 
	included by all power producing structures. These structures can overload
	the function GetProducerPriority() and return the priority for delivering
	power to the network. The library will preferentially drain power from the
	producers with highest priority, so that on-demand producers are not drained
	before power storages or steady producers. Typical return values are:
	 * Wind generator:  100
	 * Compensator:      50
	 * Steam engine:      0
	
	@author Zapper, Maikel
*/

// This object is a power producer.
public func IsPowerProducer() { return true; }

// Producer priority: the willingsness of a producer to deliver energy.
// This is high for steady producers like the wind generator and low
// for producers like the steam engine. 
public func GetProducerPriority() { return 0; }

// Destruction callback: let power network know this object is not a producer anymore.
public func Destruction()
{
	MakePowerProducer(0);
	return _inherited(...);
}
