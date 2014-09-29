// Lets cave mushrooms look frozen.

#appendto LargeCaveMushroom


public func Construction()
{
	inherited(...);
	SetMeshMaterial("FrozenCaveMushroom", 0);
}