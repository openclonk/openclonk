/*--- Life Gem ---*/

func Initialize()
{
	AddEffect("Sparkle", this, 10, 2, this);
	return 1;
}

func FxSparkleTimer(target, effect_number, effect_time)
{
	if(this()->Contained()) return;
	CreateParticle("MagicRing", 0, 0, 0, 0, Cos(effect_time*10, 100), RGBa(255,20,20,100), this, false);
	return true;
}

public func ControlUse(object clonk, int ix, int iy)
{
	// applies the healing effect even when the Clonk is at full HP
	// does this because you can block one source of damage
	AddEffect("GemHealing", clonk, 10, 4, nil, this->GetID());
	clonk->Sound("Breathing", false, 50, nil);
	this->RemoveObject();
	return true;
}

func FxGemHealingTimer(target, effect_number, effect_time)
{
	if(target->GetEnergy() >= target->GetPhysical("Energy", nil)/1000)
	{
		if(effect_time < 36) return 0;
		return -1;
	}
	target->DoEnergy(500, true);
	CreateParticle("MagicSpark", AbsX(target->GetX()) + RandomX(-8,8), AbsY(target->GetY()) + RandomX(-10,10), 0, -2, 200, RGB(255,200,200), target, Random(2));
	if(!Random(5))
		CreateParticle("MagicFire", AbsX(target->GetX()) + RandomX(-8,8), AbsY(target->GetY()) + RandomX(-5,10), 0, -2, 50, RGB(200,255,255), target, Random(2));
}

func FxGemHealingDamage(target, effect_number, damage, cause)
{
	if(damage >= 0) return damage;
	RemoveEffect(nil, nil, effect_number);
	
	// can actually block one source of damage - use wisely
	return 0;
}