/*
	Tree Trunk 2
	Authors: Win, Clonkonaut
*/

local front;

private func Initialize()
{
	front = CreateObject(Trunk2_Front, 11, 2, GetOwner());
	front->Set(this);
}

local Plane = 200;
local Name = "$Name$";