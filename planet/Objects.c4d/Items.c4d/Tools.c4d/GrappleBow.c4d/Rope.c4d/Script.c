/*
	Grapple Rope
	Author: Maikel	

	The rope used for grappling devices.
	Call BreakRope() to snap the rope.
	Calls "OnRopeBreak" in both action targets when the rope snaps.
	Rope snaps on overtension and bending (>5 vertices).
	TODO: mix both, breaking should depend on a combination of both tension and bending.
*/

/*-- Rope length --*/

local length; // The physical length of the rope.

// The maximal physical length of the rope.
private func RopeMaxLength() { return 400; }

// Functions to modify the physical rope length.
public func GetLength() { return length; }
public func SetLength(int newlength) { length = BoundBy(newlength, 0, RopeMaxLength()); return; }
public func DoLength(int dolength) { length += dolength; length = BoundBy(length, 0, RopeMaxLength()); return; }

// Call this to set physical rope length to vertex length.
public func SetToVertexLength() { length = VertexLength(); return; }

// Calculates the the vertex length of the rope.
private func VertexLength()
{
	var len = 0;
	for (var i = 0; i < GetVertexNum() - 1; i++)
		len += Distance (
			GetVertex (i, VTX_X),
			GetVertex (i, VTX_Y),
			GetVertex (i + 1, VTX_X),
			GetVertex (i + 1, VTX_Y));
	return len;
}

/*-- Rope connecting --*/

// Connects two objects to the rope, but the length will vary on their positions.
public func ConnectFree(object obj1, object obj2)
{
	SetAction("ConnectFree", obj1, obj2);
	return;
}

// Connects two objects to the rope, which will try to keep constant length.
public func ConnectPull(object obj1, object obj2)
{
	length = VertexLength();
	SetAction("ConnectPull", obj1, obj2);
	return;
}

/*-- Rope pulling --*/

private func RopeStrength() { return 40; } // How much a rope can maximally be stretched in % of the physical length.
private func RopeElasticity() { return 100; } // Defines stretching of the rope.

protected func PullObjects()
{
	// Break rope on bending -> more than 5 vertices.
	if (GetVertexNum() > 5)
	{
		Log("Rope break: overbending");
		return BreakRope();
	}

	// Check rope strength.
	if (100 * dif > RopeStrength() * (length + 10))
	{
		Log("Rope break: overtension");
		return BreakRope();
	}

	// Only if vertex length larger than physical length.
	var vlen = VertexLength();
	var dif = vlen - length;
	if (dif < 0)
	{
		length = vlen;
		return;
	}

	// Rope to short.
	if (vlen < 12)
		return BreakRope();

	// Get objects.
	var obj1 = GetActionTarget(0);
	var obj2 = GetActionTarget(1);

	// Object positions.
	var o1x = obj1->GetX();
	var o1y = obj1->GetY();
	var o2x = obj2->GetX();
	var o2y = obj2->GetY();

	// Get containers.
	while (obj1->Contained()) 
		obj1 = obj1->Contained();
	while (obj2->Contained()) 
		obj2 = obj2->Contained();

	// Check movability.
	var mov1 = 50;
	var mov2 = 50;
	if (obj1->Stuck() || obj1->GetCategory() & (C4D_StaticBack | C4D_Structure))
	{
		mov1 = 0;
		mov2 *= 2;
	}
	if (obj2->Stuck() || obj2->GetCategory() & (C4D_StaticBack | C4D_Structure))
	{
		mov1 *= 2;
		mov2 = 0;
	}
	if (!mov1 && !mov2)
		return;

	// Get vertex coordinates.
	var v1x = GetVertex(1, VTX_X);
	var v1y = GetVertex(1, VTX_Y);
	var v2x = GetVertex(GetVertexNum() - 2, VTX_X);
	var v2y = GetVertex(GetVertexNum() - 2, VTX_Y);

	// Angles.
	var ang1 = Angle(v1x, v1y, o1x, o1y);
	var ang2 = Angle(v2x, v2y, o2x, o2y);

	// Mass.
	var mass1 = Max(5, obj1->GetMass());
	var mass2 = Max(5, obj2->GetMass());

	// Calculate rope acceleration.
	var acc1 = 200 * dif * mov1 * RopeElasticity();
	var acc2 = 200 * dif * mov2 * RopeElasticity();
	acc1 /= 10 * mass1 * (length + 10);
	acc2 /= 10 * mass2 * (length + 10);
	
	// Artificial energy loss.
	var xdir1 = 39 * obj1->GetXDir(1000) / 40;
	var ydir1 = 39 * obj1->GetYDir(1000) / 40;
	var xdir2 = 39 * obj2->GetXDir(1000) / 40;
	var ydir2 = 39 * obj2->GetYDir(1000) / 40;

	// Accelerate objects.
	obj1->SetXDir(xdir1 - Sin(ang1, acc1), 1000);
	obj1->SetYDir(ydir1 + Cos(ang1, acc1), 1000);
	obj2->SetXDir(xdir2 - Sin(ang2, acc2), 1000);
	obj2->SetYDir(ydir2 + Cos(ang2, acc2), 1000);

	return;
}

/*-- Other --*/

protected func Initialize()
{
	SetVertex(0, VTX_X, GetX()); SetVertex(0, VTX_Y, GetY());
	SetVertex(1, VTX_X, GetX()); SetVertex(1, VTX_Y, GetY());
	return;
}

// Call this to break the rope.
public func BreakRope()
{
	var act1 = GetActionTarget(0);
	var act2 = GetActionTarget(1);
	SetAction("Idle");
	// notify action targets.
	if (act1)
		act1->~OnRopeBreak();
	if (act2)
		act2->~OnRopeBreak();
	RemoveObject();
	return;
}

protected func Definition(def) {
	SetProperty("Name", "$Name$", def);
	SetProperty("LineColors", [RGB(66,33,00) , RGB(66,33,00)], def);
	SetProperty("ActMap", {
		ConnectFree = {
			Prototype = Action,
			Name = "ConnectFree",
			Procedure = DFA_CONNECT,
			FacetBase = 1,
			NextAction = "ConnectFree",
		},
		ConnectPull = {
			Prototype = Action,
			Name = "ConnectPull",
			Procedure = DFA_CONNECT,
			Length = 1,
			Delay = 1,
			FacetBase = 1,
			NextAction = "ConnectPull",
			StartCall = "PullObjects",
		},
	}, def);
}