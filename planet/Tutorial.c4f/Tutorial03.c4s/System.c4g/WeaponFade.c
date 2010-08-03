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
	_inherited(obj,...);

}

func FxFadeTimer(object target, int num, int time)
{
	if(Contained() != nil)
	{
		SetObjAlpha(255);
		return -1;
	}
	if(time > 180) target->SetObjAlpha(255 - ((time - 180) * 2));
	if(time >= 307)
	{
		Enter(FindObject(Find_ID(Clonk),Find_OCF(OCF_Alive)));	
		return -1;
	}
}