// Inform scenario script that player destroys something invincible
global func FxIntInvincibleDamage(target)
{
	GameCall("OnInvincibleDamage", target);
	return 0;
}
