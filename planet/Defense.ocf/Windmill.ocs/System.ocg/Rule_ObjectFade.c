#appendto Rule_ObjectFade

func Timer()
{
	for (var fade in FindObjects(Find_Or(Find_Category(C4D_Object), Find_ID(Arrow), Find_ID(Javelin)), Find_NoContainer(), Find_Not(Find_OCF(OCF_HitSpeed1)))) 
	{
		if (!CheckFadeConditions(fade)) continue;
		if (GetEffect("IntFadeOut*", fade)) continue;
		AddEffect("IntFadeOutCandidate", fade, 1, 36, this, Rule_ObjectFade);
	}
}

func CheckFadeConditions(object fade)
{
	if (fade->GetID() == Airship) return true;
	return _inherited(fade);
}

public func FxIntFadeOutTimer(object target, effect, int time) 
{
	if (time < fade_time * 2/3) 
		return;

	if ((!(target->Contained()) && effect.x == target->GetX() && effect.y == target->GetY()) || target->~FadeOutForced()) // some objects must always fade out (e.g. Airships)
	{
		if (time >= fade_time) 
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

	target->SetClrModulation(((fade_time - time) * effect.alpha / (fade_time/3)) << 24 | effect.color);
	return 1;
}