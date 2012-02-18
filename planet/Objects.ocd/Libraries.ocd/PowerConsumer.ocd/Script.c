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
	if(GetEffect("ShowPowerMessage", this))
		FatalError("OnNotEnoughPower() called two times in a row!");
	AddEffect("ShowPowerMessage", this, 1, 10, this);
}

// called when consumer was sleeping but power is available again
public func OnEnoughPower()
{
	PowerConsumer_has_power = true;
	
	// remove symbol
	if(GetEffect("ShowPowerMessage", this))
		RemoveEffect("ShowPowerMessage", this);
}

public func FxShowPowerMessageTimer(target, effect, time)
{
	if(effect.Interval < 35*3)
		effect.Interval = 35*3;
	var t = CreateObject(FloatingMessage, 0, 0, NO_OWNER);
	t->SetMessage("{{Library_PowerConsumer}}<c ff0000>?</c>");
	t->SetYDir(-10);
}

func Destruction()
{
	MakePowerProducer(0);
	return _inherited(...);
}