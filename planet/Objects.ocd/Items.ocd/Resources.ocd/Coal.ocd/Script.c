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

public func IsFuel() { return true; }
public func GetFuelAmount(int requested_amount)
{
    // disregard the parameter, because only a complete chunk should be removed 
	if (this != Coal) return GetCon();
	return 100;
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local BlastIncinerate = 5;
local ContactIncinerate = 1;
local Plane = 460;