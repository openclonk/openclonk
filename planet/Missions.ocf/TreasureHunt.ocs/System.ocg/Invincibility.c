// Helper functions to make stuff invincible / indestructible

global func MakeInvincible()
{
	if (!this) return;
	if (!GetEffect("IntInvincible", this)) AddEffect("IntInvincible", this, 1, 0);
	return true;
}

global func FxIntInvincibleDamage(target)
{
	GameCall("OnInvincibleDamage", target);
	return 0;
}

global func FxIntInvincibleSaveScen(object obj, proplist fx, proplist props)
{
	// this is invincible. Save to scenario.
	props->AddCall("Invincible", obj, "MakeInvincible");
	return true;
}