#appendto Pump

// Pump can only pump water

func Initialize()
{
	SetPumpableMaterials("Water");
	return _inherited(...);
}