/**
	Insect Library
	Some standard behaviour to flying insects. Action with DFA_FLOAT required.
	If at any time you want to block insect behaviour, set a "Wait" command (but beware of in liquid check still be made).

	@author Clonkonaut
--*/

public func IsAnimal() { return true; }
public func IsInsect() { return true; }

public func Place(int amount, proplist area)
{
	// No calls to objects, only definitions
	if (GetType(this) == C4V_C4Object) return;

	if (!area) area = Shape->LandscapeRectangle();

	var insects = CreateArray(), insect, position;
	for (var i = 0 ; i < amount ; i++)
	{
		position = FindLocation(Loc_InArea(area), Loc_Wall(CNAT_Bottom), Loc_Sky());
		if (!position)
			position = FindLocation(Loc_InArea(area), Loc_Wall(CNAT_Bottom), Loc_Tunnel());
		if (position)
		{
			insect = CreateObjectAbove(this, position.x, position.y - 2, NO_OWNER);
			if (insect->Stuck()) insect->RemoveObject();
			if (insect) insects[GetLength(insects)] = insect;
		}
		insect = nil;
		position = nil;
	}
	return insects;
}

/** Maximum travelling distance for one 'mission' (random target point).
	0 = a random point of the whole landscape
*/
local lib_insect_max_dist = 0;

/** Set true if the insect does its business at night.
	If there's no day/night cycle (Time object), all insects never sleep.
*/
local lib_insect_nocturnal = false;

/** If true, the insect will flee from clonks.
*/
local lib_insect_shy = false;

/** Overload to define any kind of 'sitting' state for the insect (e.g. a certain action).
	Must resolve itself otherwise the insect will cease to do anything.
*/
private func IsSitting() {}

/** Return true when the insect is asleep.
*/
public func IsSleeping()
{
	// Already returns true when the insect is still searching for a resting place.
	return lib_insect_going2sleep || lib_insect_sleeping;
}

/** What to do when arrived at a random target point.
	Overload as necessary.
*/
private func MissionComplete()
{
	// Set a dummy command. After it finishes, regular Activity() behaviour will kick in.
	SetCommand("Wait", nil, nil, nil, nil, 20 + Random(80));
}

/** Overload to define any kind of interesting spot for the insect. Return true if spot was found.
	coordinates.x and coordinates.y will be coordinates to fly to.
*/
private func GetAttraction(proplist coordinates) { return false; }

/** Default sleeping behaviour: Move to a sleeping place and stay there.
	Overload as necessary.
*/
private func Sleep()
{
	if (CheckDanger()) return;
	// Sleeping phase just begun
	if (!lib_insect_going2sleep)
	{
		// Cancel current mission.
		SetCommand("None");
		lib_insect_going2sleep = true;
	}
	// As place to sleep is needed
	if (!GetCommand() && !lib_insect_sleeping)
	{
		var coordinates = {x = 0, y = 0};
		// Priority is always to move to a resting place
		if (!GetRestingPlace(coordinates))
		{
			// Default place is just at ground level
			coordinates.x = BoundBy(GetX() + Random(10) - 5, 10, LandscapeWidth()-10);
			coordinates.y = GetHorizonHeight(coordinates.x);
		}
		SetCommand("MoveTo", nil, coordinates.x, coordinates.y, nil, true);
		AppendCommand("Call", this, nil, nil, nil, nil, "SleepComplete");
	}
}

/** What to do when arrived at a resting place.
	Overload as necessary but be sure to set lib_insect_sleeping true.
*/
private func SleepComplete()
{
	SetCommand("None");
	SetComDir(COMD_None);
	SetXDir(0);
	SetYDir(0);
	lib_insect_sleeping = true;
}

/** Overload to define any kind of resting place for the insect. Return true if place was found.
	coordinates.x and coordinates.y will be coordinates to fly to.
*/
private func GetRestingPlace(proplist coordinates) { return false; }

/** Called once when the insect just wakes up.
*/
private func WakeUp()
{
	SetCommand("None");
	lib_insect_going2sleep = false;
	lib_insect_sleeping = false;
}

// Moves towards a resting place
local lib_insect_going2sleep = false;
// Arrived at resting place and sleeps
local lib_insect_sleeping = false;
// Is fleeing from a danger (interrupts sleep)
local lib_insect_escape = false;

/* Contact calls */

/** Overloaded as necessary. As well as other directions.
	Beware of the special characteristic of ContactBottom.
*/
private func ContactBottom()
{
	if (lib_insect_going2sleep) return SleepComplete();

	SetCommand("None");
	SetComDir(COMD_Up);
}
private func ContactTop()
{
	if (lib_insect_sleeping) return;

	SetCommand("None");
	SetComDir(COMD_Down);
}
private func ContactLeft()
{
	if (lib_insect_sleeping) return;

	SetCommand("None");
	SetComDir(COMD_Right);
}
private func ContactRight()
{
	if (lib_insect_sleeping) return;

	SetCommand("None");
	SetComDir(COMD_Left);
}

/* Internal stuff */

private func Initialize()
{
	AddTimer("Activity");
	SetComDir(COMD_None);
	MoveToTarget();
}

private func Death()
{
	RemoveTimer("Activity");
	SetCommand("None");
	SetComDir(COMD_None);
}

// Insects are to tiny to be thrown at!
private func QueryCatchBlow()
{
	return true;
}

// They are also too tiny to be hit by arrows etc.
public func IsProjectileTarget()
{
	return false;
}

private func Activity()
{
	if (Contained()) return;

	// It's assumed being underwater is bad for insects
	if (InLiquid())
	{
		if (GetCommand()) SetCommand("None");
		return SetComDir(COMD_Up);
	}
	// Sitting? Wait.
	if (IsSitting()) return;
	// Go somewhere if no mission
	if (!GetCommand())
	{
		// Escape ended
		if (lib_insect_escape) lib_insect_escape = false;
		// Go to sleep?
		if (Time->HasDayNightCycle())
		{
			if (!lib_insect_nocturnal && Time->IsNight())
				return Sleep();
			if (lib_insect_nocturnal && Time->IsDay())
				return Sleep();
			if (IsSleeping())
				WakeUp();
		}
		if (!CheckDanger()) MoveToTarget();
	}
}

// Set a new mission
private func MoveToTarget()
{
	var coordinates = {x = 0, y = 0};
	// Priority is always to move to an interesting spot
	if (!GetAttraction(coordinates))
	{
		// Insect may have died
		if (!this || !GetAlive()) return;

		if (!lib_insect_max_dist)
			coordinates.x = Random(LandscapeWidth());
		else
		{
			coordinates.x = GetX();
			// Prevent insect from flying around at borders too much
			if (GetX() < lib_insect_max_dist / 2) coordinates.x += lib_insect_max_dist / 2;
			if (GetX() > LandscapeWidth() - lib_insect_max_dist / 2) coordinates.x -= lib_insect_max_dist / 2;

			coordinates.x = BoundBy(coordinates.x + Random(lib_insect_max_dist) - lib_insect_max_dist / 2, 10, LandscapeWidth() - 10);
		}
		// Move to a place slightly above the surface.
		coordinates.y = GetHorizonHeight(coordinates.x, GetY()) - 30 - Random(60);
	}
	// Insect may have died
	if (!this || !GetAlive()) return;

	SetCommand("MoveTo", nil, coordinates.x, coordinates.y, nil, true);
	AppendCommand("Call", this, nil, nil, nil, nil, "MissionComplete");
}

private func GetHorizonHeight(int x, int y_current)
{
	var height = y_current;
	while (height < LandscapeHeight() && !GBackSemiSolid(AbsX(x), AbsY(height)))
		height += 5;
	return height;
}

private func CheckDanger()
{
	if (!lib_insect_shy) return false;
	// Just flee from the first found clonk
	var danger_zone = 40;
	if (IsSleeping()) danger_zone /= 2;
	var clonk = FindObject(Find_Distance(danger_zone), Find_OCF(OCF_CrewMember), Find_OCF(OCF_Alive), Find_NoContainer());
	if (clonk)
	{
		Flee(clonk);
		return true;
	}
	return false;
}

private func Flee(object from)
{
	if (!from) return;
	if (IsSleeping()) WakeUp();
	// Find a point that's somewhere up and away from the object
	var x, y;
	y = GetY() - 50 + Random(20);
	if (GetX() < from->GetX())
		x = GetX() + 50 - Random(20);
	else
		x = GetX() - 50 + Random(20);
	x = BoundBy(x, 10, LandscapeWidth()-10);
	y = BoundBy(y, 10, LandscapeHeight()-10);
	SetCommand("MoveTo", nil, x, y, nil, true);
	lib_insect_escape = true;
}