/*
	Walkable Trunk
	Authors: Win, Clonkonaut
*/

local front;

private func Initialize()
{
	front = CreateObject(WalkableTrunk_Front, 11, 2, GetOwner());
	front->Set(this);
}

local Plane = 200;
local Name = "$Name$";