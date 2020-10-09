/*-- CrystalShield --*/


public func Initialize()
{
	AddEffect("Selfdestruction", this, 100, 4 + Random(2), this, this->GetID());
	return;
}

func SetClrModulation(int color, int overlay_id)
{
	inherited(color, overlay_id, ...);
	var effect = GetEffect("Selfdestruction", this);
	if (effect)
	{
		effect.particles.R = (color >> 16) & 0xff;
		effect.particles.G = (color >>  8) & 0xff;
		effect.particles.B = (color >>  0) & 0xff;
	}
}

func FxSelfdestructionStart(object target, proplist effect, temp)
{
	if (temp) return;
	effect.particles =
	{
		Prototype = Particles_Spark(),
		Size = PV_Random(1, 3)
	};
}

func FxSelfdestructionTimer(object target, proplist effect, int timer)
{
	CreateParticle("Magic", PV_Random(-4, 4), PV_Random(-4, 4), PV_Random(-3, 3), PV_Random(-3, 3), PV_Random(10, 30), effect.particles, 3);
 	if (timer > 175) target->RemoveObject();
 	return 1;
}

