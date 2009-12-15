
protected func Construction()
{
	// parallaxity
	this["Parallaxity"] = [0,0];
	// visibility
	this["Visibility"] = VIS_Owner;
}

public func Update()
{
	var val = GetWealth(GetOwner());
	
	CustomMessage(Format("@%d",val), this, GetOwner(), 0, 75);
}