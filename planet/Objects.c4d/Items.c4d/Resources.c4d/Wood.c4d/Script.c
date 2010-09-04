/*--- The Log ---*/

protected func Hit()
{
	Sound("WoodHit*");
	return 1;
}

func Incineration()
{
	SetClrModulation (RGB(48, 32, 32));
}

public func IsFuel() { return 1; }
public func GetFuelAmount() { return 30; }

func Definition(def) {
	SetProperty("Collectible", 1, def);
	SetProperty("Name", "$Name$", def);
	SetProperty("Description", "$Description$", def);
}
