// Awards the easter egg if the player finds the windbag.

#appendto WindBag

// Award achievement if the wind bag has been collected.
public func Entrance(object container)
{
	if (container->GetOCF() & OCF_CrewMember)
	{
		// Only perform events on first wall jump.
		if (GetEffect("IntAwardedAchievement", this))
			return _inherited(container, ...);
		AddEffect("IntAwardedAchievement", this, 100);
		// Add some stars effect to the clonk indicating the easteregg.
		container->CreateParticle("StarSpark", PV_Random(-3, 3), PV_Random(-14, -10), PV_Random(-5, 5), PV_Random(-8, 0), 25, Particles_Magic(), 20);		
		// Achievement: easter egg found.
		GainScenarioAchievement("TutorialEasterEgg");
	}
	return _inherited(container, ...);
}

protected func FxIntAwardedAchievementStart(object target, proplist effect, int temporary)
{
	// Just a an effect which should always stay and hence accept always.
	return FX_OK;
}