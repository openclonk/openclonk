#appendto BalloonDeployed

public func FxControlFloatTimer(object target, proplist effect, int time)
{
	// Balloon deflates if any vertex has contact
	if (GetContact(-1, CNAT_Bottom))
	{
		Deflate();
		return FX_Execute_Kill;
	}
	return _inherited(target, effect, time, ...);
}

private func Deflate()
{
	if (GetAction() != "Deflate")
	{
		SetAction("Deflate");
		SetComDir(COMD_None);
		if (this.rider)
		{
			var fx = this.rider->~GetAI();
			if (!fx) return;
			// Tell rider to get a new target
			fx.target = nil;
			this.rider->SetCommand();
		}
	}
}

public func OnProjectileHit()
{
	if (this.rider)
	{
		var fx = this.rider->~GetAI();
		if (!fx) return;
		fx.parachute_lost = true; // rider must get a new target as soon as he lands
	}
	_inherited(...);
}