#appendto Ruby

// also flash when contained so noone can smuggle his ruby to the goal
func FxSparkleTimer(target, effect, effect_time)
{
	if (!Random(2)) return FX_OK;
	var obj = Contained() ?? this;
	obj->CreateParticle("MagicRing", 0, 0, 0, 0, effect.Interval, effect.particles, 1);
	return FX_OK;
}