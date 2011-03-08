/*--- Metal ---*/

protected func Construction()
{
	if(GBackSemiSolid())
		SetGraphics("Old");
}

protected func Hit()
{
	Sound("MetalHit*");
	return 1;
}

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
local Rebuy = true;
