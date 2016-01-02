// Control wipf balloons differently.

#appendto BalloonDeployed

public func FxControlFloatTimer(object target, proplist effect, int time)
{
	if (rider->GetID() != Wipf)
		return _inherited(target, effect, time, ...);
	
	var speed = -2;
	// Normalize vertical speed.
	if (GetYDir() > speed) SetYDir(GetYDir() - 1);
	if (GetYDir() < speed) SetYDir(GetYDir() + 1);
	return FX_OK;
}