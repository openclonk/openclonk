#appendto Arrow
#appendto LeadShot

public func SetStackCount(int amount)
{
	count = MaxStackCount();
	Update();
}

public func Hit()
{
	if(GetEffect("HitCheck",this))
		RemoveObject();
}

public func HitObject(object obj)
{
	inherited(obj,...);
	RemoveObject();
}

func UpdatePicture()
{
	SetGraphics("1");
}