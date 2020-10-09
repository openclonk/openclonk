#appendto Ropeladder_Grabber

public func IsInteractable(object clonk) { if (GetEffect("NoInteract",this)) return false; else return true;}

public func FxNoInteractTime()
{
	return -1;
}

public func Initialize() { AddEffect("NoInteract",this, 100, 26*36, this); }