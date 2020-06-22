#appendto Arrow

// Arrows do not tumble
public func TumbleStrength() { return 0; }


// Callback from the bow
public func Launch(int angle, int str, object shooter, object weapon)
{
	if (weapon && weapon.king_size)
	{
		AddEffect("KingSizeArrow", this, 100, 1, this);
		SetClrModulation(RGB(255, 128, 0));
	}
	// Forward to the arrow for other functionality.
	return _inherited(angle, str, shooter, weapon, ...);
}


public func FxKingSizeArrowTimer(object arrow, proplist fx, int time)
{
	arrow->CreateParticle("Fire", 0, 0, PV_Random(-10, 10), PV_Random(-10, 10), PV_Random(10, 20), Particles_Glimmer(), 5);
	fx.timer++;
	if (!arrow->GetXDir() && !arrow->GetYDir())
	{
		fx.timer = Max(fx.timer, 65);
	}
	if (fx.timer > 90)
	{
		arrow->Explode(RandomX(15, 22));
	}
}