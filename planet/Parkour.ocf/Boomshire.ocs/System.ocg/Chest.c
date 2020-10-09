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
			R = PV_Cos(PV_Random(0,   360, 0, 3), 127, 128),
			G = PV_Cos(PV_Random(120, 480, 0, 3), 127, 128),
			B = PV_Cos(PV_Random(240, 600, 0, 3), 127, 128),
			Phase = PV_Step(1, PV_Random(0, 7), PV_Random(1, 3)),
			DampingX = 900,
			DampingY = 900,
			ForceY = PV_Gravity(500),
			Size = PV_Random(1, 4),
			Rotation = PV_Random(0, 359),
			Alpha = PV_KeyFrames(0,
				0,    255,
				900,  255,
				1000, 0), // fade out at end only
			OnCollision = PC_Stop(),
			CollisionDensity = 25, // also collide with water
			CollisionVertex = 0,
		};
	}
	CreateParticle("Confetti", PV_Random(-5, 5), PV_Random(0, 2), PV_Random(-30, 30), PV_Random(-80,-20), PV_Random(40, 80), this.confetti, 15);
}