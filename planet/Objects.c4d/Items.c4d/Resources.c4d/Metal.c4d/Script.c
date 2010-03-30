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

func Definition(def) {
	SetProperty("Name", "$Name$", def);
}
