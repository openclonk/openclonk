/*-- Walkable Trunk Front --*/

#include Library_Cover

local back;

private func Initialize()
{
	this.cover_area = Shape->Rectangle(-118, -31, 236, 62);
	_inherited(...);
}

public func Set(object trunk)
{
	back = trunk;
	AddTimer("CheckPosition", 5);
}

private func CheckPosition()
{
	if (!back) return RemoveObject();
	if (GetX() != back->GetX() || GetY() != back->GetY())
		SetPosition(back->GetX(), back->GetY());
}

func EditCursorMoved()
{
	// Move main trunk along with front in editor mode
	if (back) back->SetPosition(GetX(), GetY());
	return true;
}

func SaveScenarioObject() { return false; }

local Plane = 505;