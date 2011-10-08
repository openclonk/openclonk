/*-- Coal --*/

protected func Construction()
{
	var graphic = Random(5);
	if(graphic)
		SetGraphics(Format("%d",graphic));
}

protected func Hit(x, y)
{
	StonyObjectHit(x,y);
	return true;
}

public func IsFuel() { return 1; }
public func GetFuelAmount() { return 80; }

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Rebuy = true;
