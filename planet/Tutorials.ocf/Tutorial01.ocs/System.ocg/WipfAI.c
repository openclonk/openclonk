// Artificial intelligence for the wipf and also the elevator.

#appendto Wipf

public func EnableTutorialControl()
{
	RemoveEffect("IntActivity", this);
	AddEffect("TutorialWipf", this, 1, 5, this);
	return;
}

public func DisableTutorialControl()
{
	RemoveEffect("TutorialWipf", this);
	AddEffect("IntActivity", this, 1, 10, this);
	return;
}

public func StartMoving()
{
	var effect = GetEffect("TutorialWipf", this);
	effect.start_moving = true;
}

protected func FxTutorialWipfStart(object target, proplist effect, int temp)
{
	if (temp)
		return FX_OK;
	effect.sequence = "Introduction";
	effect.start_moving = false;
	this.Collectible = false;
	return FX_OK;
}

protected func FxTutorialWipfTimer(object target, proplist effect, int time)
{
	// Wait for the introduction to finish and then move through the first hole.
	if (effect.sequence == "Introduction")
	{
		if (effect.start_moving)
		{
			SetAction("Walk");
			SetComDir(COMD_Right);
			SetCommand("MoveTo", nil, 344, 612);
			ScheduleCall(this, "Jump", 70);
			effect.sequence = "WaitInFrontOfHole";
			FindObject(Find_ID(ElevatorCase))->SetMoveDirection(COMD_Down, true, false);
		}
	}		
	// Wait in front of the the first hole.
	if (effect.sequence == "WaitInFrontOfHole")
	{
		if (FindObject(Find_OCF(OCF_CrewMember), Find_InRect(AbsX(256), AbsY(576), 80, 40)))
		{
			SetCommand("MoveTo", nil, 548, 604);
			effect.sequence = "MoveThroughHole";
		}
	}		
	// Wait after the first hole until the clonk arrives and move through the second.
	if (effect.sequence == "MoveThroughHole")
	{
		if (FindObject(Find_OCF(OCF_CrewMember), Find_InRect(AbsX(424), AbsY(464), 64, 64)))
		{
			SetCommand("MoveTo", nil, 780, 604);
			AddCommand("MoveTo", nil, 676, 684);
			effect.sequence = "MoveAcross";
		}
	}	
	// Wait after the second hole until the clonk arrives and move through the third.
	if (effect.sequence == "MoveAcross")
	{
		if (FindObject(Find_OCF(OCF_CrewMember), Find_InRect(AbsX(664), AbsY(560), 80, 48)))
		{
			SetCommand("MoveTo", nil, 890, 636);
			effect.sequence = "MoveToLift";	
		}
	}	
	// Move to the elevator.
	if (effect.sequence == "MoveToLift")
	{
		if (Inside(GetX(), 885, 895) && Inside(GetY(), 632, 640) && !GetCommand())
		{
			SetCommand("None");
			SetPosition(891, 634);
			effect.sequence = "WaitOnLift";	
			Schedule(FindObject(Find_ID(ElevatorCase)), "SetMoveDirection(COMD_Up, true, false)", 15);
			Schedule(FindObject(Find_ID(ElevatorCase)), "SetMoveDirection(COMD_Down, true, false)", 17);
			Schedule(FindObject(Find_ID(ElevatorCase)), "SetMoveDirection(COMD_Up, true, false)", 20);
		}
	}
	// Wait on the moving lift.
	if (effect.sequence == "WaitOnLift")
	{
		if (Inside(GetX(), 885, 895) && Inside(GetY(), 320, 328))
		{
			SetCommand("MoveTo", nil, 992, 324);
			effect.sequence = "MoveToCabin";
		}
	}
	// Move to the cabin.
	if (effect.sequence == "MoveToCabin")
	{
		
	}
	return FX_OK;
}

protected func FxTutorialWipfStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return FX_OK;
	this.Collectible = true;
	return FX_OK;
}