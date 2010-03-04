#appendto Arrow

public func SetStackCount(int amount)
{
	count = MaxStackCount();
	Update();
}

public func Hit()
{
	RemoveObject();
}

public func HitObject(object obj)
{
	inherited(obj,...);
	Hit();
}