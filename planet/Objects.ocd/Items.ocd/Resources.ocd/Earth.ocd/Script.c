/*--- Earth ---*/

protected func Construction()
{
	var graphic = Random(5);
	if(graphic)
		SetGraphics(Format("%d",graphic));
}

protected func Hit()
{
	
	CastPXS("Earth", 200, 18);
	Sound("GeneralHit?");
	RemoveObject();
	return 1;
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";