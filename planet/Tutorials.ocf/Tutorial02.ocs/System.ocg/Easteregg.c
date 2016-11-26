// The wipf will produce pink bubbles when eating a berry.

#appendto Wipf


protected func RejectCollect(id object_id)
{
	if (object_id == Sproutberry)
	{
		// Add some stars effect to the clonk indicating the easteregg.
		CreateParticle("StarSpark", PV_Random(-3, 3), PV_Random(-14, -10), PV_Random(-5, 5), PV_Random(-8, 0), 25, Particles_Magic(), 20);
		// Add the bubbles effect for the wipf.
		AddEffect("IntBreathBubbles", this, 100, 5, this);
		// Achievement: easter egg found.
		GainScenarioAchievement("TutorialEasterEgg");
	}
	return _inherited(object_id, ...);	
}

protected func FxIntBreathBubblesTimer(object target, proplist effect, int time)
{
	if (!Random(4))
		return FX_OK;
		
	var bubble = 
	{
		CollisionVertex = 500,
		OnCollision = PC_Die(),
		ForceY = PV_Gravity(-100),
		ForceX = PV_Wind(100),
		DampingX = 900, DampingY = 900,
		Alpha = PV_Linear(255, 0),
		R = 255, G = 20, B = 147,
		Size = 3,
		Phase = 0
	};	
	CreateParticle("SphereSpark", 3 * GetDir(), -2, PV_Random(-2, 2), PV_Random(-8, 0), 25, bubble, 1);	
	return FX_OK;
}