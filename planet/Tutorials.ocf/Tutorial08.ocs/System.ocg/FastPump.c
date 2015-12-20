// Make the pump a bit faster.

#appendto Pump

public func GetPumpSpeed()
{
	// Four times faster.
	return 4 * _inherited(...);
}