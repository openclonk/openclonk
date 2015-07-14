/*-- Tree Trunk 2 Front --*/

#include Library_Cover

local back;

private func Initialize()
{
	this.cover_area = Rectangle(-118, -31, 236, 62);
	_inherited();
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

local Plane = 505;