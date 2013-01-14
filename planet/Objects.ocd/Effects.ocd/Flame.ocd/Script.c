local Name = "$Name$";
local Description = "$Description$";

public func Initialize()
{
	Incinerate();
	AddTimer("Burning");
	return true;
}

func Burning()
{
	if(GetCon() > 50)
	if(!Random(3))
	{
		var x = Random(15);
		var o = CreateObject(GetID(), 0, 0, GetOwner());
		o->SetSpeed(x, -7);
		o->SetCon(GetCon()/2);
		SetSpeed(-x, -7);
		SetCon(GetCon()/2);
		
	}
}