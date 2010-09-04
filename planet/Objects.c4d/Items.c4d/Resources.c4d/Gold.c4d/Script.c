/*--- Gold ---*/

protected func Construction()
{
	var graphic = Random(5);
	if(graphic)
		SetGraphics(Format("%d",graphic));
}

protected func Hit()
{
	Sound("RockHit*");
	return 1;
}

func Definition(def) {
	SetProperty("Name", "$Name$", def);
	SetProperty("Collectible", 1, def);
	SetProperty("Description", "$Description$", def);
}
