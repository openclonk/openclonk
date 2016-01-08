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

public func OnProjectileHit(object projectile)
{
	// Pop the balloon and tumble the rider.
	CreateParticle("Air", 0, -10, PV_Random(-10, 10), PV_Random(-10, 10), 10, Particles_Air(), 30);
	Sound("Objects::Balloon::Pop");
	if (parent)
		parent->RemoveObject();
	// Drop the rider and set its killer in case it tumbles out of the map.
	if (rider)
	{
		rider->SetSpeed(GetXDir(), GetYDir());
		rider->SetAction("Jump");
	}
	RemoveObject();
}