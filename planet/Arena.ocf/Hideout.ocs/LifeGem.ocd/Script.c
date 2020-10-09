/*--- Life Gem ---*/

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";

// does not fade out. Who wants to leave it lying around anyway!
func HasNoFadeOut(){return true;}

func Initialize()
{
	AddEffect("Sparkle", this, 1, 30 + RandomX(-3, 3), this);
	return 1;
}

func FxSparkleStart(target, effect, temp)
{
	if (temp) return;
	var color = this->~GetGemColor() ?? RGB(255, 20, 20);
	effect.particles =
	{
		Prototype = Particles_MagicRing(),
		R = (color >> 16) & 0xff,
		G = (color >>  8) & 0xff,
		B = (color >>  0) & 0xff,
	};
}

func FxSparkleTimer(target, effect, effect_time)
{
	if (this->Contained() || !Random(2)) return FX_OK;
	CreateParticle("MagicRing", 0, 0, 0, 0, effect.Interval, effect.particles, 1);
	return FX_OK;
}

public func ControlUse(object clonk, int ix, int iy)
{
	// applies the healing effect even when the Clonk is at full HP
	// does this because you can block one source of damage
	AddEffect("GemHealing", clonk, 10, 4, nil, this->GetID());
	clonk->Sound("Clonk::Action::Breathing", false, 50, nil);
	this->RemoveObject();
	return true;
}

func FxGemHealingStart(target, effect, temp)
{
	if (temp) return;
	effect.glimmer_particles =
	{
		Prototype = Particles_Glimmer(),
		R = 255,
		G = 200,
		B = 200
	};
	
	effect.sparks = 
	{
		Prototype = Particles_Spark(),
		Size = PV_Random(1, 3),
		R = 255,
		G = PV_Random(180, 220),
		B = PV_Random(180, 220)
	};
}

func FxGemHealingTimer(target, effect, effect_time)
{
	if (target->GetEnergy() >= target->GetMaxEnergy())
	{
		if (effect_time < 36) return 0;
		return -1;
	}
	
	target->DoEnergy(500, true);

	target->CreateParticle("Magic", PV_Random(-5, +5), PV_Random(-8, 8), PV_Random(-1, 1), PV_Random(-10, -5), PV_Random(10, 20), effect.glimmer_particles, 3);
	if (!Random(10)) effect.switcher = !effect.switcher;
	if (effect.switcher)
		target->CreateParticle("MagicSpark", PV_Random(-3, 3), PV_Random(0, 8), PV_Random(-1, 1), PV_Random(-2, -1), PV_Random(20, 30), effect.sparks, 2);
}

func FxGemHealingDamage(target, effect, damage, cause)
{
	if (damage >= 0) return damage;
	RemoveEffect(nil, target, effect);
	
	// can actually block one source of damage - use wisely
	return 0;
}
