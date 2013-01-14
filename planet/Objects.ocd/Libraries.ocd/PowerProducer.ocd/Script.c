/**
	Power producer
	Should be included by all power generators.
*/

public func IsPowerProducer() { return true; }

func Destruction()
{
	MakePowerProducer(0);
	return _inherited(...);
}
