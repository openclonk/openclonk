#appendto CableHoist

public func Construction()
{
	var res = _inherited(...);
	SetCableSpeed(3);
	return res;
}