// Golden barrel for the cannon.

#appendto Cannon


public func Construction()
{
	_inherited(...);
	SetMeshMaterial("GoldenCannon", 0);
	MakeInvincible();
}
