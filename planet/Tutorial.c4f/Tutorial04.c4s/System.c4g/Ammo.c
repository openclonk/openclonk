// Infinite ammo for bow and javelin.
// Fade out for arrow and javelin.

#appendto Arrow
#appendto Javelin

// Infinite ammo.
public func SetStackCount(int amount)
{
	count = MaxStackCount();
	Update();
}

public func UpdatePicture()
{
	SetGraphics("1");
}

// Ammo fades out after some time.
protected func Hit()
{
	AddEffect("IntFadeOut", this, 100, 1, this);	
	return _inherited(...);
}

public func HitObject()
{
	AddEffect("IntFadeOut", this, 100, 1, this);	
	return _inherited(...);
}

protected func FxIntFadeOutStart(object target, int num) 
{
	EffectVar(0, target, num) = target->GetClrModulation() & 0x00ffffff;
	EffectVar(1, target, num) = target->GetClrModulation() >> 24 & 255;
}

protected func FxIntFadeOutTimer(object target, int num, int time)
{
	if (time < 144 * 2/3) 
		return 1;
	if (!target->Contained()) 
	{
		if (time >= 144)
		{
			target->RemoveObject();
			return -1;
		}
	}
	else
	{
		target->SetClrModulation(EffectVar(1, target, num) << 24 | EffectVar(0, target, num));
		return -1;
	}
	target->SetClrModulation(((144 - time) * EffectVar(1, target, num) / (144/3)) << 24 | EffectVar(0, target, num));
	return 1;
}

protected func FxIntFadeOutEffect(string new_effect_name) 
{
	if (new_effect_name == "IntFadeOut")
		return -1;
}