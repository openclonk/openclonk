// Fix the catapults to their position.

#appendto Catapult

public func Initialize()
{
	SetCategory(C4D_StaticBack);
	AddVertex(0, 18);
	return _inherited(...);
}

// Block any turning.
public func ControlLeft(object clonk) { return true; }
public func ControlRight(object clonk) { return true; }
public func SetDir(int dir) { return; }