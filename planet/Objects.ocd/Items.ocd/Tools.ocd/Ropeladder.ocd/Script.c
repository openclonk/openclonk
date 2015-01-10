/*
	Ropeladder
	Author: Randrian

	A ladder consisting of ropesegments. The physics is done completly intern with point masses.
	Verlet integration and simple stick constraints are used for the rope physics.
	The segments are an extra object, which handels the graphics and the detection of the ladder by the clonk.

	Interface:
	Unroll(int dir, int unrolldir, int length);	// dir == -1 expand to the left, dir == 1 expand right;
																							// unrolldir == COMD_Down, COMD_Right, COMD_Left or COMD_Up specifies where to adjust to a wall
																							// length, length of segments, default 15
*/

#include Library_Rope

static const Ladder_MaxParticles = 15;//30;//15*3;
static const Ladder_Iterations = 10;
static const Ladder_Precision = 100;
static const Ladder_SegmentLength = 5;//2;

local particles;
local segments;
local TestArray;

local MaxSegmentCount;

local SwitchRopes;
local MirrorSegments;

local UnrollDir;

local ParticleCount;

local grabber;

public func ControlUse(object clonk, int x, int y)
{
	if(!clonk->GetContact(-1)) return true;
	// Unroll dir
	var dir = -1;
	if(x > 0) dir = 1;
	if(clonk->GetAction() == "Scale")
	{
		if(clonk->GetDir() == 0)
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
	else if(clonk->GetAction() == "Hangle")
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
	for(var i = 1; i < GetLength(segments); i++)
	{
		segments[i]->SetGraphics("Line", GetID(), 2, 1);
		segments[i]->SetGraphics("Line", GetID(), 3, 1);
		segments[i]->SetGraphics(nil, nil, 4);
		segments[i]->SetGraphics(nil, nil, 5);
		if(i > 1)
			segments[i]->SetGraphics("NoRope", segments[i]->GetID(), 4, 1);
		if(i == GetLength(segments)-1)
			segments[i]->SetGraphics("NoRope", segments[i]->GetID(), 5, 1);
		if(i == 1)
		{
			segments[i]->SetGraphics("Anchor", GetID(), 1, 1);
			segments[i]->SetGraphics("AnchorOverlay", GetID(), 5, 1);
		}
	}
}

func TestMoveOut(xdir, ydir)
{
	if(!Stuck()) return;
	for(var i = 0; i < 8; i++)
	{
		if(!GBackSolid(i*xdir, i*ydir))
		{
			SetPosition(GetX()+i*xdir, GetY()+i*ydir);
			if(!Stuck())
				break;
			SetPosition(GetX()-i*xdir, GetY()-i*ydir);
		}
	}
}

public func Unroll(int dir, int unrolldir, int length)
{
	if(!unrolldir) unrolldir = COMD_Down;
	SwitchRopes = 0;
	// Unroll dir
	if(unrolldir == COMD_Left || unrolldir == COMD_Right)
	{
		var xdir = 1;
		if(unrolldir == COMD_Right)
		{
			xdir = -1;
		}
		var x1 = 0;
		var x2 = 0;
		var x0 = 0;
		var y1 =-5;
		var y2 = 5;
		while(!GBackSolid(x0,  0) && Abs(x0) < 10) x0 += xdir;
		while(!GBackSolid(x1, y1) && Abs(x1) < 10) x1 += xdir;
		while(!GBackSolid(x2, y2) && Abs(x2) < 10) x2 += xdir;

		SetPosition(GetX()+x0-2*xdir, GetY());
		SetR(90*xdir+Angle(x1, y1, x2, y2));
	}
	else if(unrolldir == COMD_Up)
	{
		var x1 =-2;
		var x2 = 2;
		var y1 = 0;
		var y2 = 0;
		var y0 = 0;
		while(!GBackSolid( 0, y0) && Abs(y0) < 10) y0--;
		while(!GBackSolid(x1, y1) && Abs(y1) < 10) y1--;
		while(!GBackSolid(x2, y2) && Abs(y2) < 10) y2--;

		SetPosition(GetX(), GetY()+y0+2);
		SetR(-90+Angle(x2, y2, x1, y1));
		SwitchRopes = 1;
	}
	else
	{
		var x1 =-2;
		var x2 = 2;
		var y1 = 0;
		var y2 = 0;
		var y0 = 0;
		while(!GBackSolid( 0, y0) && Abs(y0) < 10) y0++;
		while(!GBackSolid(x1, y1) && Abs(y1) < 10) y1++;
		while(!GBackSolid(x2, y2) && Abs(y2) < 10) y2++;

		SetPosition(GetX(), GetY()+y0-2);
		SetR(90+Angle(x2, y2, x1, y1));
	}
	if(!length)
	{
		if(MaxSegmentCount == nil)
			MaxSegmentCount = Ladder_MaxParticles;
	}
	else MaxSegmentCount = length;
	DoUnroll(dir);
}

public func MakeBridge(obj1, obj2)
{
	MirrorSegments = 1;
	SetProperty("Collectible", 0);
	StartRopeConnect(obj1, obj2);
	AddEffect("IntHang", this, 1, 1, this);
}//MakeBridge(Object(221), Object(150))

protected func DoUnroll(dir)
{
	MirrorSegments = dir;
	UnrollDir = dir;
	SetAction("Hanging");
	SetProperty("Collectible", 0);

//	TestArray = [[0, 1], [1, 0], [1, 1], [0, 2], [1, 2], [2, 0], [2, 1], [2, 2], [0, 3], [1, 3], [2, 3], [3, 0], [3, 1], [3, 2], [0, 4], [1, 4], [2, 4], [3, 3], [4, 0], [4, 1], [4, 2], [0, 5], [1, 5], [2, 5], [3, 4], [3, 5], [4, 3], [4, 4], [5, 0], [5, 1], [5, 2], [5, 3], [0, 6], [1, 6], [2, 6], [3, 6], [4, 5], [5, 4], [6, 0], [6, 1], [6, 2], [6, 3], [0, 7], [1, 7], [2, 7], [3, 7], [4, 6], [5, 5], [5, 6], [6, 4], [6, 5], [7, 0], [7, 1], [7, 2], [7, 3], [0, 8], [1, 8], [2, 8], [3, 8], [4, 7], [4, 8], [5, 7], [6, 6], [7, 4], [7, 5], [8, 0], [8, 1], [8, 2], [8, 3], [8, 4], [0, 9], [1, 9], [2, 9], [3, 9], [4, 9], [5, 8], [6, 7], [7, 6], [7, 7], [8, 5], [9, 0], [9, 1], [9, 2], [9, 3], [9, 4]];

	grabber = CreateObjectAbove(Ropeladder_Grabber);
	grabber->SetAction("Attach", this);

	StartRope();

	AddEffect("IntHang", this, 1, 1, this);

	AddEffect("UnRoll", this, 1, 2, this);
}

func StartRollUp() { RemoveEffect("UnRoll", this); AddEffect("RollUp", this, 1, 1, this); }

func FxUnRollTimer()
{
	if(ParticleCount == MaxSegmentCount)
	{
		if(GetActTime() < MaxSegmentCount*2*2) return;
		// If it wasn't possible to acchieve at least half the full length we pull in again
		if( -(particles[0][0][1]-particles[ParticleCount-1][0][1]) < (ParticleCount*Ladder_SegmentLength*Ladder_Precision)/2)
			StartRollUp();
		return -1;
	}
	AddSegment(UnrollDir*Ladder_Precision, 0);
}

func FxRollUpTimer() { if(ParticleCount == 0) return -1; RemoveSegment(); }

func FxIntHangTimer()
{
	TimeStep();
	if(!Stuck()) TestLength();
}

func TestLength()
{
	if(GetActTime() < 36) return;
	// If it wasn't possible to acchieve at least half the full length we pull in again
	if( -(particles[0][0][1]-particles[ParticleCount-1][0][1]) < (ParticleCount*5*Ladder_Precision)/2)
		StartRollUp();
}

/* --------------------- Callbacks form the rope ---------------------- */

/* To be overloaded for special segment behaviour */
private func CreateSegment(int index, object previous)
{
	var segment;
	if(index == 0)
	{
		segment = CreateObjectAbove(Ropeladder_Segment);
		segment->SetGraphics("None");
		segment->SetMaster(this, 0);
	}
	else
	{
		segment = CreateObjectAbove(Ropeladder_Segment);

		segment->SetMaster(this, ParticleCount);

		segment->SetNextLadder(previous);
		previous->SetPreviousLadder(segment);

		segment->SetGraphics("None");
	}
	return segment;
}

private func DeleteSegment(object segment, previous)
{
	if(segment)
		segment->RemoveObject();
	if(previous)
		previous->SetPreviousLadder(nil);
}

/* When the last segment is removed */
private func RopeRemoved()
{
	RemoveEffect("IntHang", this);
	SetCategory(C4D_Object);
	SetAction("Idle");
	SetProperty("Collectible", 1);

	// Try to move the Ropeladder somewhere out if it is stuck
	TestMoveOut( 0, -1); // Up
	TestMoveOut( 0, +1); // Down
	TestMoveOut(-1,  0); // Left
	TestMoveOut(+1,  0); // Right

	grabber->RemoveObject();

	SetGraphics("", GetID());
}

/* --------------------- Graphics of segments ---------------------- */
func UpdateLines()
{
	var oldangle;
	for(var i=1; i < ParticleCount; i++)
	{
		// Update the Position of the Segment
		segments[i]->SetPosition(GetPartX(i), GetPartY(i));

		// Calculate the angle to the previous segment
		var angle;
		if(i >= 1)
		{
			angle = Angle(particles[i][0][0], particles[i][0][1], particles[i-1][0][0], particles[i-1][0][1]);
			segments[i]->SetAngle(angle);
		}

		// Every segment has not its graphics, but the graphics of the previous segment (or achor for the first)
		// Otherwise the drawing order would be wrong an we would get lines over segments
		
		// Draw the segment as an overlay for the following segment (only the last segment has two graphics (its and the previous)
		if(i > 1)
			SetLineTransform(segments[i], -oldangle, particles[i-1][0][0]*10-GetPartX(i)*1000,particles[i-1][0][1]*10-GetPartY(i)*1000, 1000, 4, MirrorSegments );
		if(i == ParticleCount-1)
			SetLineTransform(segments[i], -angle, particles[i][0][0]*10-GetPartX(i)*1000,particles[i][0][1]*10-GetPartY(i)*1000, 1000, 5, MirrorSegments );

		// The first segment has to draw the achor too
		if(i == 1)
		{
			SetLineTransform(segments[i], -GetR(), GetX()*1000-GetPartX(i)*1000, GetY()*1000-GetPartY(i)*1000, 1000, 1);
			SetLineTransform(segments[i], -GetR(), GetX()*1000-GetPartX(i)*1000, GetY()*1000-GetPartY(i)*1000, 1000, 5);
		}

		// Draw the left line
		var start = GetRopeConnetPosition(i, 0, 0, angle, oldangle);
		var end   = GetRopeConnetPosition(i, 0, 1, angle, oldangle);

		var diff = Vec_Sub(end,start);
		var diffangle = Vec_Angle(diff, [0,0]);
		var point = Vec_Add(start, Vec_Div(diff, 2));
		var length = Vec_Length(diff)*1000/Ladder_Precision/8;

		SetLineTransform(segments[i], -diffangle, point[0]*10-GetPartX(i)*1000,point[1]*10-GetPartY(i)*1000, length, 2 );

		// Draw the right line
		var start = GetRopeConnetPosition(i, 1, 0, angle, oldangle);
		var end   = GetRopeConnetPosition(i, 1, 1, angle, oldangle);
		
		var diff = Vec_Sub(end,start);
		var diffangle = Vec_Angle(diff, [0,0]);
		var point = Vec_Add(start, Vec_Div(diff, 2));
		var length = Vec_Length(diff)*1000/Ladder_Precision/8;

		SetLineTransform(segments[i], -diffangle, point[0]*10-GetPartX(i)*1000,point[1]*10-GetPartY(i)*1000, length, 3 );

		// Remember the angle
		oldangle = angle;
	}
}

static const Ropeladder_Segment_LeftXOffset = 200;
static const Ropeladder_Segment_RightXOffset = -100;

func GetRopeConnetPosition(int index, bool fRight, bool fEnd, int angle, int oldangle)
{
	if(SwitchRopes && index == 1 && fEnd == 0) fRight = !fRight;
	if(!(index == 1 && fEnd == 0) && MirrorSegments == -1) fRight = !fRight;
	if(fEnd == 0)
	{
		var start = [0,0];
		if(fRight == 0)
		{
			if(index >= 2)
			{
				start = particles[index-1][0][:];
				start[0] += -Cos(oldangle, Ropeladder_Segment_LeftXOffset*MirrorSegments);
				start[1] += -Sin(oldangle, Ropeladder_Segment_LeftXOffset*MirrorSegments);
			}
			else
			{
				start = [GetX()*Ladder_Precision, GetY()*Ladder_Precision];
				start[0] += -Cos(GetR(), 188)+Sin(GetR(), 113);
				start[1] += -Sin(GetR(), 188)-Cos(GetR(), 113);
			}
		}
		else
		{
			if(index >= 2)
			{
				start = particles[index-1][0][:];
				start[0] += -Cos(oldangle, Ropeladder_Segment_RightXOffset*MirrorSegments);
				start[1] += -Sin(oldangle, Ropeladder_Segment_RightXOffset*MirrorSegments);
			}
			else
			{
				start = [GetX()*Ladder_Precision, GetY()*Ladder_Precision];
				start[0] += +Cos(GetR(), 188)+Sin(GetR(), 113);
				start[1] += +Sin(GetR(), 188)-Cos(GetR(), 113);
			}
		}
		return start;
	}
	else
	{
		var end = [0,0];
		if(fRight == 0)
		{
			end = particles[index][0][:];
			end[0] += -Cos(angle, Ropeladder_Segment_LeftXOffset*MirrorSegments);
			end[1] += -Sin(angle, Ropeladder_Segment_LeftXOffset*MirrorSegments);
		}
		else
		{
			end = particles[index][0][:];
			end[0] += -Cos(angle, Ropeladder_Segment_RightXOffset*MirrorSegments);
			end[1] += -Sin(angle, Ropeladder_Segment_RightXOffset*MirrorSegments);
		}
		return end;
	}
}

func SetLineTransform(obj, int r, int xoff, int yoff, int length, int layer, int MirrorSegments) {
	if(!MirrorSegments) MirrorSegments = 1;
	var fsin=Sin(r, 1000), fcos=Cos(r, 1000);
	// set matrix values
	obj->SetObjDrawTransform (
		+fcos*MirrorSegments, +fsin*length/1000, xoff,
		-fsin*MirrorSegments, +fcos*length/1000, yoff,layer
	);
}

/* --------------------- Clonk on ladder ---------------------- */
public func OnLadderGrab(clonk, index)
{
	// Do some speed when the clonk jumps on the ladder
	if(index == 0) return;
	particles[index][0][0] += BoundBy(clonk->GetXDir()/2, -25, 25)*Ladder_Precision;
}

public func OnLadderClimb(clonk, index)
{
	// The clonk drags on the upper segments and pushes on the lower ones
	if(index > 2 && index < ParticleCount-3)
	{
		particles[index-2][0][0] -= 1*Ladder_Precision/5*(-1+2*clonk->GetDir());
		particles[index+2][0][0] += 1*Ladder_Precision/5*(-1+2*clonk->GetDir());
	}
	else if(index > 2 && index < ParticleCount-2)
	{
		particles[index-2][0][0] -= 1*Ladder_Precision/5*(-1+2*clonk->GetDir());
		particles[index+1][0][0] += 1*Ladder_Precision/5*(-1+2*clonk->GetDir());
	}
}

public func GetLadderData(index)
{
	var startx = particles[index][0][0]*10;
	var starty = particles[index][0][1]*10;
	if(index == 0)
	{
		var angle = Angle(particles[2][0][0], particles[2][0][1], particles[0][0][0], particles[0][0][1]);
		return [startx, starty, startx, starty-5000, angle];
	}
	if(index == ParticleCount-1 || segments[index+1]->~CanNotBeClimbed())
	{
		angle = Angle(particles[index][0][0], particles[index][0][1], particles[index-2][0][0], particles[index-2][0][1]);
	}
	else
		angle = Angle(particles[index+1][0][0], particles[index+1][0][1], particles[index-1][0][0], particles[index-1][0][1]);
	var endx = particles[index-1][0][0]*10;
	var endy = particles[index-1][0][1]*10;
	return [startx, starty, endx, endy, angle];
}

func Hit()
{
	Sound("WoodHit?");
}

// Save unrolled ladders in scenario
public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (UnrollDir) props->AddCall("Unroll", this, "Unroll", UnrollDir);
	return true;
}

public func IsTool() { return true; }
public func IsToolProduct() { return true; }

local ActMap = {
Hanging = {
	Prototype = Action,
	Name = "Hanging"
},
};
local Name = "$Name$";
local UsageHelp = "$UsageHelp$";
local Description = "$Description$";
local Collectible = 1;
local Rebuy = true;
