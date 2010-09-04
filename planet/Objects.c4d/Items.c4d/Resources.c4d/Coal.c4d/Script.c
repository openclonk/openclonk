/*-- Coal --*/

protected func Construction()
{
	var graphic = Random(5);
	if(graphic)
		SetGraphics(Format("%d",graphic));
}

protected func Hit()
{
	Sound("RockHit*");
}

func Definition(def) {
	SetProperty("Collectible", 1, def);
	SetProperty("Name", "$Name$", def);
	SetProperty("Description", "$Description$", def);
}
