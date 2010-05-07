#appendto Arrow
#appendto Javelin

public func Hit()
{
	AddEffect("Fade",this,1,1,this);
	return _inherited(...);
}

public func HitObject(object obj)
{
	if(obj->GetOCF() & OCF_CrewMember) return;
	inherited(obj,...);

}

func FxFadeTimer(object target, int num, int time)
{
	if(Contained() != nil) return -1;
	if(time > 180) target->SetObjAlpha(255 - ((time - 180) * 2));
	if(time >= 307)
	{
		RemoveObject();
		return -1;
	}
}