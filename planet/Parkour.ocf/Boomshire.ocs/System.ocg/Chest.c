#appendto Chest

public func IsContainer() { return false; }
public func RejectCollect() { return true; }

// Start the chest closed.
public func Construction()
{
	inherited(...);

	PlayAnimation("Close", 5, Anim_Linear(0, 0, GetAnimationLength("Close"), 1, ANIM_Hold));
	is_open = false;
}

// Never automatically open/close the chest.
public func OnShownInInteractionMenuStart(bool last) { }
public func OnShownInInteractionMenuStop(bool last) { }

// Open chests with space here so that you can actually see the confetti effect
public func GetInteractionMetaInfo(object clonk)
{
	return { Description = Format("$OpenChest$", GetName()) };
}

public func IsInteractable(object clonk)
{
	return !is_open;
}

public func Interact(object clonk)
{
	if (!is_open) return Open(clonk);
}

public func Open(clonk)
{
	GameCall("OnChestOpened", clonk);
	Sound("Toot");
	ScheduleCall(this, "DoTheConfetti", 2, 20);
	return inherited();
}

public func DoTheConfetti()
{
	if (!this.confetti)
	{
		this.confetti =
		{
			CollisionVertex = 500,
			OnCollision = PC_Stop(),
			ForceX = PV_Random(-5, 5, 10),
			ForceY = PV_Gravity(100),
			Size = 1,
			R = PV_Random(100,255),
			G = PV_Random(100,255),
			B = PV_Random(100,255),
			DampingX = 900, DampingY = 900
		};
	}
	CreateParticle("Confetti", PV_Random(-5, 5), PV_Random(0, 2), PV_Random(-30, 30), PV_Random(-80,-20), PV_Random(100, 150), this.confetti, 15);
}