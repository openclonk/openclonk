#appendto Chest

local opened;

public func IsContainer() { return false; }

public func IsInteractable() { return !opened; }

public func GetInteractionMetaInfo(object clonk)
{
	return { Description = "$OpenChest$" };
}

public func Interact(object clonk)
{

	Open();
	opened = true;
	GameCall("OnChestOpened", clonk);
	Sound("Toot");
	var confetti =
	{
		CollisionVertex = 500,
		OnCollision = PC_Stop(),
		ForceX = PV_Random(-2, 2, 5),
		ForceY = PV_Gravity(100),
		Size = 1,
		R = PV_Random(100,255),
		G = PV_Random(100,255),
		B = PV_Random(100,255)
	};
	CreateParticle("Confetti", 0, -3, PV_Random(-5, 5), PV_Random(-20,-10), 150, confetti, 25);
	return true;
}