/*-- Black Powder --*/

protected func Incineration()
{
	Schedule("Explode(20)", 90);
}

func Definition(def) {
	SetProperty("Collectible", 1, def);
	SetProperty("Name", "$Name$", def);
	SetProperty("Description", "$Description$", def);
}
