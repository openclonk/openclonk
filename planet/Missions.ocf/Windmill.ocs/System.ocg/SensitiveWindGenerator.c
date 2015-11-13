#appendto WindGenerator

func Damage()
{
	Explode(30);
}

// No lightbulbs
func IsPowerProducer() { return false; }

private func RegisterPowerProduction(int amount) {}

//No triangles
func RedrawFlagRadius() { return; }