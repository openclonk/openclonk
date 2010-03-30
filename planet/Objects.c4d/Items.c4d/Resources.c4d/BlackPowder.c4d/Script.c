/*-- Black Powder --*/

#strict 2

protected func Incineration()
{
	Schedule("Explode(20)", 90);
}

func IsAlchemyProduct() { return 1; }
func AlchemyProcessTime() { return 100; }

func Definition(def) {
	SetProperty("Collectible", 1, def);
	SetProperty("Name", "$Name$", def);
}
