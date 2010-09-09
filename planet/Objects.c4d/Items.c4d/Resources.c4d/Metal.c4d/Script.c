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
