/*-- Arrow target --*/

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}

public func IsProjectileTarget(target,shooter)
{
	return 1;
}

public func OnProjectileHit()
{
	Fireworks();
	return 1;
}
