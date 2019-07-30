/* Castle Floor */

local breakiness;

// Configure which parts of the floor can fall apart
// 1 = left, 2 = right, 3 = both
func SetBreaky(int level)
{
	breakiness = level;
	SetAction("Be");
	AddTimer("CheckBurden", 4);
}

private func CheckBurden()
{
	if (!breakiness) return RemoveTimer("CheckBurden");

	if (breakiness >= 2)
		if (FindObject(Find_InRect(10,-18, 18, 10), Find_ID(Clonk)))
			BreakRight();
	if (breakiness == 1 || breakiness == 3)
		if (FindObject(Find_InRect(-28,-18, 18, 10), Find_ID(Clonk)))
			BreakLeft();
}

private func BreakRight()
{
	Sound("Environment::Tree::Crack");
	CastObjects(Wood, 2, 15, 20, 1, 180, 90);
	if (GetPhase() == 1)
	{
		SetPhase(3);
		SetSolidMask(261, 14, 87, 6, 0, 0);
	}
	else
	{
		SetPhase(2);
		SetSolidMask(174, 14, 87, 6, 0, 0);
	}
	breakiness -= 2;
}

private func BreakLeft()
{
	Sound("Environment::Tree::Crack");
	CastObjects(Wood, 2, 15, -20, 1, 180, 90);
	if (GetPhase() == 2)
	{
		SetPhase(3);
		SetSolidMask(261, 14, 87, 6, 0, 0);
	}
	else
	{
		SetPhase(1);
		SetSolidMask(87, 14, 87, 6, 0, 0);
	}
	breakiness -= 1;
}

local ActMap = {
	Be = {
		Prototype = Action,
		Name = "Be",
		X = 0,
		Y = 0,
		Wdt = 87,
		Hgt = 14,
		Length = 4,
		Delay = 0,
	},
};