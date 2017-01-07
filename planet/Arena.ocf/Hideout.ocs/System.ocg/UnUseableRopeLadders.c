#appendto Ropeladder_Grabber

public func IsInteractable(object clonk)
{
	return !GetEffect("NoInteract", this);
}

public func FxNoInteractTime() // TODO: Fix this error
{
	return -1;
}

public func Initialize()
{
	AddEffect("NoInteract", this, 100, 10*36, this);
}