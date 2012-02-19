/**
	Power consumer
	Cares about showing the "No Power"-symbol
	and provides CurrentlyHasPower()
	The rest is handled by Library_Power
*/

local PowerConsumer_has_power = 0;


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
public func OnNotEnoughPower()
{
	PowerConsumer_has_power = false;
	
	// show symbol
	this->AddStatusSymbol(Library_PowerConsumer);
}

// called when consumer was sleeping but power is available again
public func OnEnoughPower()
{
	PowerConsumer_has_power = true;
	
	// remove symbol
	this->RemoveStatusSymbol(Library_PowerConsumer);
}

func Destruction()
{
	MakePowerProducer(0);
	return _inherited(...);
}