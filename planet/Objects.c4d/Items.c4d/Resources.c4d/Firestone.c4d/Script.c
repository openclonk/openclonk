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

func Definition(def) {
	SetProperty("Collectible", 1, def);
	SetProperty("Name", "$Name$", def);
}
