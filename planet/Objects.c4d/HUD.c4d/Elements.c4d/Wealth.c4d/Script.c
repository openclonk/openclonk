
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
	
	var num;
	if(val < 180) num = 4;
	if(val < 120) num = 3;
	if(val < 70) num = 2;
	if(val < 30) num = 1;
	if(val < 10) num = 0;
	
	SetGraphics(Format("%d",num));
}