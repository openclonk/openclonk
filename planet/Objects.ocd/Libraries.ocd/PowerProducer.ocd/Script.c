/**
	Power producer
	Should be included by all power generators.
*/

public func IsPowerProducer() { return true; }

local PowerProducer_last_production;

// overload global function
func MakePowerProducer(int value)
{
	PowerProducer_last_production = value;
	return _inherited(value, ...);
}

func Destruction()
{
	MakePowerProducer(0);
	return _inherited(...);
}

public func OnObjectInformationDialogueOpen(object dialogue)
{
	_inherited(dialogue, ...);
	dialogue->AddLine({type = HUD_OBJECTINFODISPLAY_TEXT, name = Format("$EnergyProduction$: <c 00ff00>%5d</c>", PowerProducer_last_production), lines = 1 });
	return true;
}