/*-- Arrow target --*/

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}

public func IsProjectileTarget(target,shooter)
{
	return 1;
}

public func QueryCatchBlow(obj)
{
//	obj->Schedule("RemoveObject", 1);
	Fireworks();
	return 1;
}
