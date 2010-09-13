// Weapons fade out after some time, and return to their owner.

#appendto Arrow
#appendto Javelin
#appendto Bow

protected func Departure(object container)
{
	if (container->GetOCF() & OCF_CrewMember)
		AddEffect("Fade", this, 100, 1, this);
	return _inherited(container, ...);
}

protected func Hit()
{
	AddEffect("Fade", this, 100, 1, this);
	return _inherited(...);
}

public func HitObject(object obj)
{
	if(obj->GetOCF() & OCF_CrewMember)
		return;
	return _inherited(obj, ...);
}

protected func FxFadeTimer(object target, int num, int time)
{
	if (Contained() != nil)
	{
		SetObjAlpha(255);
		return -1;
	}
	if (time > 210)
		target->SetObjAlpha(255 - (time - 210) * 2);
	if (time >= 330)
	{
		target->SetObjAlpha(255);
		var restorer = CreateObject(ObjectRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		restorer->SetPosition(x, y);
		var to_container = FindObject(Find_OCF(OCF_CrewMember));
		restorer->SetRestoreObject(target, to_container);
		return -1;
	}
	return 1;
}

// Only one fade effect allowed.
protected func FxFadeEffect(string new_name, object target, int num, int new_num)
{
	if (new_name == "Fade")
		return -1;
	return -2;
}