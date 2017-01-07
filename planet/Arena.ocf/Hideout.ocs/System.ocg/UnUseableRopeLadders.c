#appendto Ropeladder_Grabber

public func IsInteractable(object clonk)
{
	return !GetEffect("NoInteract", this);
}

public func FxNoInteractTimer(object target, proplist fx, int timer)
{
	return FX_Execute_Kill;
}

public func Initialize()
{
	AddEffect("NoInteract", this, 100, 10*36, this);
}