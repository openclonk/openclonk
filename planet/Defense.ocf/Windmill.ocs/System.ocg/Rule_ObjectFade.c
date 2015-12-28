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