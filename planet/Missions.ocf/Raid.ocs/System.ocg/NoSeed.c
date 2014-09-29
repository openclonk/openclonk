#appendto Library_Plant

protected func Construction(...)
{
	var r = _inherited(...);
	RemoveTimer("Seed");
	return r;
}