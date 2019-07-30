/* Goldbarren direkt einsammeln */

#appendto GoldBar

public func Construction(...)
{
	this.Description = Format("$SpecialGoldBarDesc$", MAX_GOLD_BARS);
	return _inherited(...);
}

public func IsFoundryProduct() { return false; } // Don't let players produce them

func Entrance(container)
{
	if (container->GetAlive())
	{
		DoSellEffect(container);
		GameCall("OnGoldBarCollected", container);
		RemoveObject();
		return true;
	}
	return _inherited(container, ...);
}

func DoSellEffect(container)
{
	var value = 0;
	var fm = CreateObjectAbove(FloatingMessage, 0, 0, NO_OWNER);
	fm->SetColor(250, 200, 50);
	fm->FadeOut(2, 10);
	fm->SetSpeed(0, -5);
	fm->SetMessage("+1</c>{{GoldBar}}");
	container->Sound("UI::Cash");
	
	var dust_particles =
	{
		Prototype = Particles_Dust(),
		Size = PV_KeyFrames(0, 0, 0, 100, 10, 1000, 0),
		Alpha = PV_KeyFrames(0, 0, 255, 750, 255, 1000, 0),
		R = 200,
		G = 125,
		B = 125,
	};
	
	var flash_particles =
	{
		Prototype = Particles_Flash(),
		Size = 20
	};
	
	CreateParticle("Flash", 0, 0, 0, 0, 8, flash_particles);
	CreateParticle("Dust", 0, 0, PV_Random(-10, 10), PV_Random(-10, 10), PV_Random(18, 36), dust_particles, 10);
}
