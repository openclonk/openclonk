/**
	Ropeladder
	A ladder consisting of ropesegments. The physics is done completly internally with point masses.
	Verlet integration and simple stick constraints are used for the rope physics. The segments are 
	extra objects, which handle the graphics and the detection of the ladder by the clonk.

	In a scenario you can unroll the ladder with the command Unroll(int dir, int unrolldir, int length).

	@author Randrian																				
*/

#include Library_Rope

local Ladder_MaxParticles = 15;
local Ladder_Iterations = 10;
local Ladder_Precision = 100;
local Ladder_SegmentLength = 5;
local Ropeladder_Segment_LeftXOffset = 200;
local Ropeladder_Segment_RightXOffset = -100;

local MaxSegmentCount;

local switch_ropes;
local MirrorSegments;

local UnrollDir;

local grabber;


/*-- Usage --*/

public func RejectUse(object clonk)
{
	return !clonk->GetContact(-1);
}

public func ControlUse(object clonk, int x, int y)
{
	// Unroll dir
	var dir = -1;
	if (x > 0) dir = 1;
	if (clonk->GetAction() == "Scale")
	{
		if (clonk->GetDir() == 0)
		{
			Exit(0, 8);
			Unroll(1, COMD_Right);
		}
		else
		{
			Exit(0, 8);
			Unroll(-1, COMD_Left);
		}
	}
	else if (clonk->GetAction() == "Hangle")
	{
		Exit(0, 0);
		Unroll(dir, COMD_Up);
	}
	else
	{
		Exit(0, 5);
		Unroll(dir, COMD_Down);
	}
	return true;
}

public func UpdateSegmentOverlays()
{
	for (var i = 1; i < GetLength(lib_rope_segments); i++)
	{
		lib_rope_segments[i]->SetGraphics("Line", GetID(), 2, 1);
		lib_rope_segments[i]->SetGraphics("Line", GetID(), 3, 1);
		lib_rope_segments[i]->SetGraphics(nil, nil, 4);
		lib_rope_segments[i]->SetGraphics(nil, nil, 5);
		if (i > 1)
			lib_rope_segments[i]->SetGraphics("NoRope", lib_rope_segments[i]->GetID(), 4, 1);
		if (i == GetLength(lib_rope_segments)-1)
			lib_rope_segments[i]->SetGraphics("NoRope", lib_rope_segments[i]->GetID(), 5, 1);
		if (i == 1)
		{
			lib_rope_segments[i]->SetGraphics("Anchor", GetID(), 1, 1);
			lib_rope_segments[i]->SetGraphics("AnchorOverlay", GetID(), 5, 1);
		}
	}
}

public func TestMoveOut(int xdir, int ydir)
{
	if (!Stuck()) 
		return;
	for (var i = 0; i < 8; i++)
	{
		if (!GBackSolid(i * xdir, i * ydir))
		{
			SetPosition(GetX() + i * xdir, GetY() + i * ydir);
			if (!Stuck())
				break;
			SetPosition(GetX() - i * xdir, GetY() - i * ydir);
		}
	}
}

public func Unroll(int dir, int unrolldir, int length)
{
	if (!unrolldir) 
		unrolldir = COMD_Down;
	switch_ropes = false;
	// Unroll dir
	if (unrolldir == COMD_Left || unrolldir == COMD_Right)
	{
		var xdir = 1;
		if (unrolldir == COMD_Right)
			xdir = -1;
		var x1 = 0;
		var x2 = 0;
		var x0 = 0;
		var y1 =-5;
		var y2 = 5;
		while (!GBackSolid(x0,  0) && Abs(x0) < 10) x0 += xdir;
		while (!GBackSolid(x1, y1) && Abs(x1) < 10) x1 += xdir;
		while (!GBackSolid(x2, y2) && Abs(x2) < 10) x2 += xdir;

		SetPosition(GetX() + x0 - 2 * xdir, GetY());
		SetR(90 * xdir + Angle(x1, y1, x2, y2));
	}
	else if (unrolldir == COMD_Up)
	{
		var x1 =-2;
		var x2 = 2;
		var y1 = 0;
		var y2 = 0;
		var y0 = 0;
		while (!GBackSolid( 0, y0) && Abs(y0) < 10) y0--;
		while (!GBackSolid(x1, y1) && Abs(y1) < 10) y1--;
		while (!GBackSolid(x2, y2) && Abs(y2) < 10) y2--;

		SetPosition(GetX(), GetY() + y0 + 2);
		SetR(-90 + Angle(x2, y2, x1, y1));
		switch_ropes = true;
	}
	else
	{
		var x1 =-2;
		var x2 = 2;
		var y1 = 0;
		var y2 = 0;
		var y0 = 0;
		while (!GBackSolid( 0, y0) && Abs(y0) < 10) y0++;
		while (!GBackSolid(x1, y1) && Abs(y1) < 10) y1++;
		while (!GBackSolid(x2, y2) && Abs(y2) < 10) y2++;

		SetPosition(GetX(), GetY() + y0 - 2);
		SetR(90 + Angle(x2, y2, x1, y1));
	}
	if (!length)
	{
		if (MaxSegmentCount == nil)
			MaxSegmentCount = Ladder_MaxParticles;
	}
	else 
		MaxSegmentCount = length;
	DoUnroll(dir);
}

protected func DoUnroll(dir)
{
	MirrorSegments = dir;
	UnrollDir = dir;
	SetAction("Hanging");
	SetProperty("Collectible", 0);

	grabber = CreateObjectAbove(Ropeladder_Grabber);
	grabber->SetAction("Attach", this);

	StartRope();

	AddEffect("IntHang", this, 1, 1, this);

	AddEffect("UnRoll", this, 1, 2, this);
	return;
}

public func StartRollUp()
{
	RemoveEffect("UnRoll", this); 
	AddEffect("RollUp", this, 1, 1, this);
	return;
}

public func FxUnRollTimer()
{
	if (lib_rope_particle_count == MaxSegmentCount)
	{
		if (GetActTime() < MaxSegmentCount * 4) 
			return FX_OK;
		// If it wasn't possible to acchieve at least half the full length we pull in again
		if (-(lib_rope_particles[0].y - lib_rope_particles[lib_rope_particle_count - 1].y) < (lib_rope_particle_count * Ladder_SegmentLength * Ladder_Precision) / 2)
			StartRollUp();
		return FX_Execute_Kill;
	}
	AddSegment(UnrollDir * Ladder_Precision, 0);
	return FX_OK;
}

public func FxRollUpTimer() 
{ 
	if (lib_rope_particle_count == 0) 
		return FX_Execute_Kill;
	RemoveSegment();
	return FX_OK; 
}

public func FxIntHangTimer()
{
	// Perform a step in the rope library simulation.
	TimeStep();
	if (!Stuck()) 
		TestLength();
	return FX_OK;
}

public func TestLength()
{
	if (GetActTime() < 36) 
		return;
	// If it wasn't possible to acchieve at least half the full length we pull in again.
	if (lib_rope_particles[lib_rope_particle_count - 1].y - lib_rope_particles[0].y < lib_rope_particle_count * 5 * Ladder_Precision / 2)
		StartRollUp();
	return;
}


/*-- Rope Callbacks --*/

// To be overloaded for special segment behaviour.
private func CreateSegment(int index, object previous)
{
	var segment;
	if (index == 0)
	{
		segment = CreateObjectAbove(Ropeladder_Segment);
		segment->SetGraphics("None");
		segment->SetMaster(this, 0);
	}
	else
	{
		segment = CreateObjectAbove(Ropeladder_Segment);
		segment->SetMaster(this, lib_rope_particle_count);
		segment->SetNextLadder(previous);
		previous->SetPreviousLadder(segment);
		segment->SetGraphics("None");
	}
	return segment;
}

private func DeleteSegment(object segment, object previous)
{
	if (segment)
		segment->RemoveObject();
	if (previous)
		previous->SetPreviousLadder(nil);
	return;
}

// Called when the last segment is removed.
private func RopeRemoved()
{
	RemoveEffect("IntHang", this);
	SetCategory(C4D_Object);
	SetAction("Idle");
	this.Collectible = true;

	// Try to move the ropeladder somewhere out if it is stuck.
	TestMoveOut( 0, -1); // Up
	TestMoveOut( 0, +1); // Down
	TestMoveOut(-1,  0); // Left
	TestMoveOut(+1,  0); // Right

	// Remove the ladder grabber and reset the graphics.
	grabber->RemoveObject();
	SetGraphics();
	return;
}

/*-- Segment Graphics --*/

public func UpdateLines()
{
	var oldangle = 0;
	for (var i = 1; i < lib_rope_particle_count; i++)
	{
		// Update the position of the segment.
		lib_rope_segments[i]->SetPosition(GetPartX(i), GetPartY(i));

		// Calculate the angle to the previous segment.
		var angle = Angle(lib_rope_particles[i].x, lib_rope_particles[i].y, lib_rope_particles[i - 1].x, lib_rope_particles[i - 1].y);
		lib_rope_segments[i]->SetAngle(angle);

		// Every segment does not have its own graphics, but the graphics of the previous segment (or achor for the first).
		// Otherwise the drawing order would be wrong an we would get lines over segments.
		
		// Draw the segment as an overlay for the following segment (only the last segment has two graphics (its and the previous).
		if (i > 1)
			SetLineTransform(lib_rope_segments[i], -oldangle, lib_rope_particles[i - 1].x * 10-GetPartX(i) * 1000, lib_rope_particles[i - 1].y * 10-GetPartY(i) * 1000, 1000, 4, MirrorSegments);
		if (i == lib_rope_particle_count - 1)
			SetLineTransform(lib_rope_segments[i], -angle, lib_rope_particles[i].x * 10 - GetPartX(i) * 1000, lib_rope_particles[i].y * 10 - GetPartY(i) * 1000, 1000, 5, MirrorSegments);

		// The first segment has to draw the achor too
		if (i == 1)
		{
			SetLineTransform(lib_rope_segments[i], -GetR(), GetX() * 1000 - GetPartX(i) * 1000, GetY() * 1000 - GetPartY(i) * 1000, 1000, 1);
			SetLineTransform(lib_rope_segments[i], -GetR(), GetX() * 1000 - GetPartX(i) * 1000, GetY() * 1000 - GetPartY(i) * 1000, 1000, 5);
		}

		// Draw the left line
		var start = GetRopeConnectPosition(i, 0, 0, angle, oldangle);
		var end   = GetRopeConnectPosition(i, 0, 1, angle, oldangle);

		var diff = Vec_Sub(end, start);
		var diffangle = Vec_Angle(diff, [0, 0]);
		var point = Vec_Add(start, Vec_Div(diff, 2));
		var length = Vec_Length(diff) * 125 / Ladder_Precision;

		SetLineTransform(lib_rope_segments[i], -diffangle, point[0] * 10 - GetPartX(i) * 1000, point[1] * 10 - GetPartY(i) * 1000, length, 2);

		// Draw the right line
		var start = GetRopeConnectPosition(i, 1, 0, angle, oldangle);
		var end   = GetRopeConnectPosition(i, 1, 1, angle, oldangle);
		
		var diff = Vec_Sub(end, start);
		var diffangle = Vec_Angle(diff, [0, 0]);
		var point = Vec_Add(start, Vec_Div(diff, 2));
		var length = Vec_Length(diff) * 1000 / Ladder_Precision / 8;

		SetLineTransform(lib_rope_segments[i], -diffangle, point[0] * 10 - GetPartX(i) * 1000, point[1] * 10 - GetPartY(i) * 1000, length, 3);

		// Remember the angle
		oldangle = angle;
	}
	return;
}


public func GetRopeConnectPosition(int index, bool right, bool end, int angle, int oldangle)
{
	if (switch_ropes && index == 1 && !end) 
		right = !right;
	if (!(index == 1 && !end) && MirrorSegments == -1) 
		right = !right;
	if (!end)
	{
		var ladder_start = [0, 0];
		if (!right)
		{
			if (index >= 2)
			{
				ladder_start = [lib_rope_particles[index-1].x, lib_rope_particles[index-1].y];
				ladder_start[0] += -Cos(oldangle, Ropeladder_Segment_LeftXOffset * MirrorSegments);
				ladder_start[1] += -Sin(oldangle, Ropeladder_Segment_LeftXOffset * MirrorSegments);
			}
			else
			{
				ladder_start = [GetX() * Ladder_Precision, GetY() * Ladder_Precision];
				ladder_start[0] += -Cos(GetR(), 188) + Sin(GetR(), 113);
				ladder_start[1] += -Sin(GetR(), 188) - Cos(GetR(), 113);
			}
		}
		else
		{
			if (index >= 2)
			{
				ladder_start = [lib_rope_particles[index-1].x, lib_rope_particles[index-1].y];
				ladder_start[0] += -Cos(oldangle, Ropeladder_Segment_RightXOffset * MirrorSegments);
				ladder_start[1] += -Sin(oldangle, Ropeladder_Segment_RightXOffset * MirrorSegments);
			}
			else
			{
				ladder_start = [GetX() * Ladder_Precision, GetY() * Ladder_Precision];
				ladder_start[0] += Cos(GetR(), 188) + Sin(GetR(), 113);
				ladder_start[1] += Sin(GetR(), 188) - Cos(GetR(), 113);
			}
		}
		return ladder_start;
	}
	else
	{
		var ladder_end = [0, 0];
		if (!right)
		{
			ladder_end = [lib_rope_particles[index].x, lib_rope_particles[index].y];
			ladder_end[0] += -Cos(angle, Ropeladder_Segment_LeftXOffset * MirrorSegments);
			ladder_end[1] += -Sin(angle, Ropeladder_Segment_LeftXOffset * MirrorSegments);
		}
		else
		{
			ladder_end = [lib_rope_particles[index].x, lib_rope_particles[index].y];
			ladder_end[0] += -Cos(angle, Ropeladder_Segment_RightXOffset * MirrorSegments);
			ladder_end[1] += -Sin(angle, Ropeladder_Segment_RightXOffset * MirrorSegments);
		}
		return ladder_end;
	}
}

public func SetLineTransform(object obj, int r, int xoff, int yoff, int length, int layer, int MirrorSegments)
{
	if (!MirrorSegments) 
		MirrorSegments = 1;
	var fsin = Sin(r, 1000), fcos = Cos(r, 1000);
	// Draw transform the object.
	obj->SetObjDrawTransform (
		 fcos * MirrorSegments, fsin * length / 1000, xoff,
		-fsin * MirrorSegments, fcos * length / 1000, yoff, layer
	);
	return;
}


/*-- Clonk Interaction --*/

// Perturb some segments when the clonk jumps onto the ladder
public func OnLadderGrab(object clonk, int index)
{
	if (index == 0) 
		return;
	lib_rope_particles[index].x += BoundBy(clonk->GetXDir() / 2, -25, 25) * Ladder_Precision;
	return;
}

// Called when the clonk climbs the ladder.
public func OnLadderClimb(object clonk, int index)
{
	var dir = 2 * clonk->GetDir() - 1;
	// The clonk drags on the upper segments and pushes on the lower ones
	if (index > 2 && index < lib_rope_particle_count - 3)
	{
		lib_rope_particles[index-2].x -= dir * Ladder_Precision / 5;
		lib_rope_particles[index + 2].x += dir * Ladder_Precision / 5;
	}
	else if (index > 2 && index < lib_rope_particle_count - 2)
	{
		lib_rope_particles[index-2].x -= dir * Ladder_Precision / 5;
		lib_rope_particles[index + 1].x += dir * Ladder_Precision / 5;
	}
	return;
}

public func GetLadderData(int index)
{
	var startx = lib_rope_particles[index].x * 10;
	var starty = lib_rope_particles[index].y * 10;
	var angle;
	if (index == 0)
	{
		angle = Angle(lib_rope_particles[2].x, lib_rope_particles[2].y, lib_rope_particles[0].x, lib_rope_particles[0].y);
		return [startx, starty, startx, starty - 5000, angle];
	}
	if (index == lib_rope_particle_count-1 || lib_rope_segments[index + 1]->~CanNotBeClimbed())
	{
		var to_index = index - 2;
		if (index == 1)
			to_index = 0;
		angle = Angle(lib_rope_particles[index].x, lib_rope_particles[index].y, lib_rope_particles[to_index].x, lib_rope_particles[to_index].y);
	}
	else
	{
		angle = Angle(lib_rope_particles[index + 1].x, lib_rope_particles[index + 1].y, lib_rope_particles[index - 1].x, lib_rope_particles[index - 1].y);
	}
	var endx = lib_rope_particles[index - 1].x * 10;
	var endy = lib_rope_particles[index - 1].y * 10;
	return [startx, starty, endx, endy, angle];
}

public func Hit()
{
	Sound("Hits::Materials::Wood::WoodHit?");
}

// Save unrolled ladders in scenario.
public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) 
		return false;
	if (UnrollDir) 
		props->AddCall("Unroll", this, "Unroll", UnrollDir);
	return true;
}

public func IsTool() { return true; }
public func IsToolProduct() { return true; }

/*-- Properties --*/

local ActMap = {
	Hanging = {
		Prototype = Action,
		Name = "Hanging"
	},
};
local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;
local Components = {Wood = 2/*, Rope = 1*/};