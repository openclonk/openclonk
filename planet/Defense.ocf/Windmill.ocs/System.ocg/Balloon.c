#appendto Balloon

public func ControlUseStart(object clonk)
{
	// Create the balloon and set its speed and rider.
	var balloon = CreateObjectAbove(BalloonDeployed, clonk->GetX(), clonk->GetY()+5);
	balloon->SetSpeed(clonk->GetXDir(), clonk->GetYDir());
	balloon->SetRider(clonk);
//	balloon->SetParent(this);

	// Sound.
//	Sound("Objects::Balloon::Inflate");

	// Make the clonk ride the balloon.
	clonk->SetAction("Ride", balloon);

	// Make sure clonk is not diving.
	var side = ["L", "R"][Random(2)];
	clonk->PlayAnimation(Format("Jump.%s", side), CLONK_ANIM_SLOT_Movement, Anim_Linear(clonk->GetAnimationLength("Jump.L"), 0, clonk->GetAnimationLength("Jump.L"), 36, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	return true;
}

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