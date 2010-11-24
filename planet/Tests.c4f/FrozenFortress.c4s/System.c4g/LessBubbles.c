#appendto Fx_Bubble


public func FxMoveTimer(object target, int num, int time)
{
	if (!GBackLiquid(0, -3) && !GetEffect("Fade", this) || time > 108)
		AddEffect("Fade", target, 100, 1, target);

	// Bubbles burst into smaller bubles
/*	if (!Random(30) && target->GetCon() > 100)
	{
		for (var i = 0; i < 3; i++)
		{
			var bubble = CreateObject(Fx_Bubble);
			bubble->SetCon(10 * target->GetCon() / 15);
			bubble->SetYDir(target->GetYDir());
		}
		RemoveObject();
		return -1;
	}*/

	// Jittery movement
	SetYDir(GetYDir() - 2 + Random(5));
	if (Inside(GetXDir(), -6, 6))
		SetXDir(GetXDir() + 2 * Random(2) - 1);

	return 1;
}