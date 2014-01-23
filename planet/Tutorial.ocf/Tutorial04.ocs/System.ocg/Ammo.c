// Infinite ammo for bow and javelin.
// Fade out for arrow and javelin.

#appendto Arrow
#appendto Javelin

// Infinite ammo.
protected func Construction()
{
	var ret = _inherited(...);
	SetInfiniteStackCount();
	return ret;
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

protected func FxIntFadeOutStart(object target, effect) 
{
	effect.color = target->GetClrModulation() & 0x00ffffff;
	effect.alpha = target->GetClrModulation() >> 24 & 255;
}

protected func FxIntFadeOutTimer(object target, effect, int time)
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
		target->SetClrModulation(effect.alpha << 24 | effect.color);
		return -1;
	}
	target->SetClrModulation(((144 - time) * effect.alpha / (144/3)) << 24 | effect.color);
	return 1;
}

protected func FxIntFadeOutEffect(string new_effect_name) 
{
	if (new_effect_name == "IntFadeOut")
		return -1;
}