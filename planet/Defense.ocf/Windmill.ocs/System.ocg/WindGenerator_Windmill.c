#appendto WindGenerator
#appendto Windmill

// Brutal
func Death()
{
	GameCall("WindmillDown", this);

	if (GetID() == Windmill)
	{
		var ruin = CreateObjectAbove(Ruin_Windmill, 0, GetDefBottom()-GetY(), NO_OWNER);
		ruin->MakeInvincible();
		RemoveObject();
	} else {
		CreateParticle("Planks", PV_Random(-10, 10), PV_Random(-30, 30), PV_Random(-30, 30), PV_Random(-100,-50), 500, Particles_Planks(), 15);
		RemoveObject();
	}

	return true;
}

// Overload structure library
func Damage()
{
	return;
}

// No lightbulbs
func IsPowerProducer() { return false; }

private func RegisterPowerProduction(int amount) {}
private func UnregisterPowerProduction(int amount) {}

// No triangles
func RedrawFlagRadius() { return; }

// Is objective
public func IsMainObjective() { return true; }


local ContactIncinerate = 0;
local BlastIncinerate = 0;