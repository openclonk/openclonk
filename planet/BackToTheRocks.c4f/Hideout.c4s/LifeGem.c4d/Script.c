/*--- Life Gem ---*/

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";

// does not fade out. Who wants to leave it lying around anyway!
func HasNoFadeOut(){return true;}

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
	if(target->GetEnergy() >= target.MaxEnergy/1000)
	{
		if(effect_time < 36) return 0;
		return -1;
	}
	target->DoEnergy(500, true);
	var xoff=RandomX(-5,5);
	for(var fac=-1; fac <= 1;fac+=2)CreateParticle("Magic", AbsX(target->GetX()) + fac * xoff, AbsY(target->GetY()) + RandomX(-8,8), 0, -2, 40, RGB(255,200,200), target, Random(2));
	if(!Random(10)) EffectVar(0, target, effect_number)=!EffectVar(0, target, effect_number);
	if(EffectVar(0, target, effect_number))
		CreateParticle("MagicSpark", AbsX(target->GetX()) + RandomX(-3,3), AbsY(target->GetY()) + RandomX(-0,8), 0, -2, 30, RGBa(255,55,55, 50), target, Random(2));
}

func FxGemHealingDamage(target, effect_number, damage, cause)
{
	if(damage >= 0) return damage;
	RemoveEffect(nil, target, effect_number);
	
	// can actually block one source of damage - use wisely
	return 0;
}
