#appendto Boompack

protected func FxFlightTimer(object pTarget, effect, int iEffectTime)
{
	if (g_theme != MiamiIce) return inherited(pTarget, effect, iEffectTime, ...);

	// clonk does sense the danger and with great presence of mind jumps of the rocket
	if (fuel<20 && rider)
	{
		JumpOff(rider, 30);
	}

	if (!Random(105)) Sound("Fire::Cracker");

	if (fuel<=0)
	{
		DoFireworks();
		return;
	}

	var ignition = iEffectTime % 9;
	
	if (!ignition)
	{
		var angle = GetR()+RandomX(-dirdev, dirdev);
		SetXDir(3*GetXDir()/4 + Sin(angle, 24));
		SetYDir(3*GetYDir()/4-Cos(angle, 24));
		SetR(angle);
	}
	
	var x = -Sin(GetR(), 10);
	var y = +Cos(GetR(), 10);

	var xdir = GetXDir() / 2;
	var ydir = GetYDir() / 2;
	
	if (!effect.clr)
		effect.clr = HSL(Random(255), 255, 100);
	
	CreateParticle("FireDense", x, y, PV_Random(xdir - 4, xdir + 4), PV_Random(ydir - 4, ydir + 4), PV_Random(16, 38), Particles_Colored(Particles_Thrust(), effect.clr), 5);
	
	fuel--;
}
