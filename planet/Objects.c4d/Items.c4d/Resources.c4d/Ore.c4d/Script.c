/*-- Ore --*/

protected func Construction()
{
	var graphic = Random(5);
	if(graphic)
		SetGraphics(Format("%d",graphic));
}

protected func Hit()
{
	Sound("RockHit");
}

local Collectible = 1;
local Name = "$Name$";
