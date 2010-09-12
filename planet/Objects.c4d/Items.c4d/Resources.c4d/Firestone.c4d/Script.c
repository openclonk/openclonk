/*--- Flint ---*/

protected func Construction()
{
	var graphic = Random(3);
	if(graphic)
		SetGraphics(Format("%d",graphic));
}

func Hit()
{
	Explode(20);
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
