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

protected func AddSegment()
{
	segments[ParticleCount] = CreateObject(Ropeladder_Segment);

	segments[ParticleCount]->SetMaster(this, ParticleCount);

	segments[ParticleCount]->SetNextLadder(segments[ParticleCount-1]);
	segments[ParticleCount-1]->SetPreviousLadder(segments[ParticleCount]);

	segments[ParticleCount]->SetGraphics("None");

	var oldx = particles[ParticleCount][0][0];
	var oldy = particles[ParticleCount][0][1];
	particles[ParticleCount] = [[ oldx+UnrollDir*Ladder_Precision, oldy], [ oldx, oldy], [0,1*Ladder_Precision], 1]; // Pos, Oldpos, acceleration (gravity), mass
	
	ParticleCount++;
	UpdateSegmentOverlays();
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

protected func RemoveSegment()
{
	ParticleCount--;
	
	segments[ParticleCount]->RemoveObject();

	if(ParticleCount == 0)
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
		
		return;
	}
		
	SetLength(segments, ParticleCount);
	SetLength(particles, ParticleCount);
	segments[ParticleCount-1]->SetPreviousLadder(nil);
	UpdateSegmentOverlays();
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
	if(!unrolldir) unrolldir == COMD_Down;
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

protected func DoUnroll(dir)
{
	MirrorSegments = dir;
	UnrollDir = dir;
	SetGraphics("Anchor");
	SetAction("Hanging");
	SetProperty("Collectible", 0);

	TestArray = [[0, 1], [1, 0], [1, 1], [0, 2], [1, 2], [2, 0], [2, 1], [2, 2], [0, 3], [1, 3], [2, 3], [3, 0], [3, 1], [3, 2], [0, 4], [1, 4], [2, 4], [3, 3], [4, 0], [4, 1], [4, 2], [0, 5], [1, 5], [2, 5], [3, 4], [3, 5], [4, 3], [4, 4], [5, 0], [5, 1], [5, 2], [5, 3], [0, 6], [1, 6], [2, 6], [3, 6], [4, 5], [5, 4], [6, 0], [6, 1], [6, 2], [6, 3], [0, 7], [1, 7], [2, 7], [3, 7], [4, 6], [5, 5], [5, 6], [6, 4], [6, 5], [7, 0], [7, 1], [7, 2], [7, 3], [0, 8], [1, 8], [2, 8], [3, 8], [4, 7], [4, 8], [5, 7], [6, 6], [7, 4], [7, 5], [8, 0], [8, 1], [8, 2], [8, 3], [8, 4], [0, 9], [1, 9], [2, 9], [3, 9], [4, 9], [5, 8], [6, 7], [7, 6], [7, 7], [8, 5], [9, 0], [9, 1], [9, 2], [9, 3], [9, 4]];

	grabber = CreateObject(Ropeladder_Grabber);
	grabber->SetAction("Attach", this);
	
	ParticleCount = 1;
	
	segments = CreateArray(ParticleCount);

	segments[0] = CreateObject(Ropeladder_Segment);
	segments[0]->SetGraphics("None");

	segments[0]->SetMaster(this, 0);

	particles = CreateArray(ParticleCount);
	for(var i = 0; i < MaxSegmentCount; i++)
		particles[i] = [[ (GetX()+i*dir)*Ladder_Precision, GetY()*Ladder_Precision], [ (GetX()+i*1)*Ladder_Precision, GetY()*Ladder_Precision], [0,1*Ladder_Precision], 1]; // Pos, Oldpos, acceleration (gravity), mass
	particles[0] = [[ GetX()*Ladder_Precision, GetY()*Ladder_Precision],  [(GetX()+1)*Ladder_Precision, GetY()*Ladder_Precision], [0,1*Ladder_Precision], 0];

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
	AddSegment();
}

func TestLength()
{
	if(GetActTime() < 36) return;
	// If it wasn't possible to acchieve at least half the full length we pull in again
	if( -(particles[0][0][1]-particles[ParticleCount-1][0][1]) < (ParticleCount*5*Ladder_Precision)/2)
		StartRollUp();
}

func FxRollUpTimer() { if(ParticleCount == 0) return -1; RemoveSegment(); }

func FxIntHangTimer() { TimeStep(); }

// Verlet integration step
func Verlet(fFirst)
{
	var fTimeStep = 1;
	for(var i = 1; i < ParticleCount; i++)
	{
		var x = particles[i][0];
		var temp = x;
		var oldx = particles[i][1];
		var a = particles[i][2];

		// Verlet step, get speed out of distance moved relativ to the last position
		particles[i][0][0] += x[0]-oldx[0]+a[0]*fTimeStep*fTimeStep;
		particles[i][0][1] += x[1]-oldx[1]+a[1]*fTimeStep*fTimeStep;
		particles[i][1] = temp;
	}
	particles[0][0] = [GetX()*Ladder_Precision, GetY()*Ladder_Precision];
	if(!Stuck()) TestLength();
}

func UpdateLines()
{UpdateSegmentOverlays();
	var fTimeStep = 1;
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
				start = particles[index-1][0];
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
				start = particles[index-1][0];
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
			end = particles[index][0];
			end[0] += -Cos(angle, Ropeladder_Segment_LeftXOffset*MirrorSegments);
			end[1] += -Sin(angle, Ropeladder_Segment_LeftXOffset*MirrorSegments);
		}
		else
		{
			end = particles[index][0];
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

func LogSpeed()
{
	// Helperfunction for Debugpurpose
	var array = [];
	for(var i=0; i < ParticleCount; i++)
	{
		var x = particles[i][0];
		var oldx = particles[i][1];
		array[GetLength(array)] = Distance(x[0]-oldx[0], x[1]-oldx[1]);
	}
	Log("%v", array);
}

func GetPartX(index) { return (particles[index][0][0]+Ladder_Precision/2)/Ladder_Precision; }
func GetPartY(index) { return (particles[index][0][1]+Ladder_Precision/2)/Ladder_Precision; }

public func OnLadderGrab(clonk, index)
{
	// Do some speed when the clonk jumps on the ladder
	if(index == 0) return;
	particles[index][0][0] += BoundBy(clonk->GetXDir(), -50, 50)*Ladder_Precision;
}

public func OnLadderClimb(clonk, index)
{
	// The clonk drags on the upper segments and pushes on the lower ones
	if(index > 2 && index < ParticleCount-3)
	{
		particles[index-2][0][0] -= 1*Ladder_Precision*(-1+2*clonk->GetDir());
		particles[index+2][0][0] += 1*Ladder_Precision*(-1+2*clonk->GetDir());
	}
	else if(index > 2 && index < ParticleCount-2)
	{
		particles[index-2][0][0] -= 1*Ladder_Precision*(-1+2*clonk->GetDir());
		particles[index+1][0][0] += 1*Ladder_Precision*(-1+2*clonk->GetDir());
	}
}

public func GetLadderData(index, &startx, &starty, &endx, &endy, &angle)
{
	startx = particles[index][0][0]*10;
	starty = particles[index][0][1]*10;
	if(index == 0)
	{
		endx = startx;
		endy = starty-5000;
		angle = Angle(particles[2][0][0], particles[2][0][1], particles[0][0][0], particles[0][0][1]);
		return true;
	}
	if(index == ParticleCount-1 || segments[index+1]->CanNotBeClimbed())
	{
		angle = Angle(particles[index][0][0], particles[index][0][1], particles[index-2][0][0], particles[index-2][0][1]);
	}
	else
		angle = Angle(particles[index+1][0][0], particles[index+1][0][1], particles[index-1][0][0], particles[index-1][0][1]);
	endx = particles[index-1][0][0]*10;
	endy = particles[index-1][0][1]*10;
	return true;
}

func SatisfyConstraints()
{
	for(var j=0; j < Ladder_Iterations; j++)
	{
		// Satisfy all stick constraints (move the particles to fit the length)
		for(var i=0; i < ParticleCount-1; i++)
		{
			// Keep length
			var restlength = Ladder_SegmentLength*Ladder_Precision; // normal length between laddersegments
			// Get coordinates and inverse masses
			var x1 = particles[i][0];
			var x2 = particles[i+1][0];
			var invmass1 = particles[i][3];
			var invmass2 = particles[i+1][3];
			// calculate difference
			var delta = Vec_Sub(x2,x1);
			var deltalength = Sqrt(Vec_Dot(delta,delta));
			var diff = 0;
			if(deltalength != 0) // savety against division throught zero
				diff = (deltalength-restlength)*1000/(deltalength*(invmass1+invmass2));
			// Set new positions
			particles[i][0]   = Vec_Add(x1, Vec_Div(Vec_Mul(delta, invmass1*diff), 1000));
			particles[i+1][0] = Vec_Sub(x2, Vec_Div(Vec_Mul(delta, invmass2*diff), 1000));
		}
		for(var i=0; i < ParticleCount; i++)
		{
			// Don't touch ground
			if(GBackSolid(GetPartX(i)-GetX(), GetPartY(i)-GetY()) )
			{
				// Moving left?
				var xdir = -1;
				if(particles[i][0][0] < particles[i][1][0])
					xdir = 1;
				var ydir = -1;
				// Moving up?
				if(particles[i][0][1] < particles[i][1][1])
					ydir = 1;
				// Look for all possible places where the particle could move (from nearest to farest)
				for(var pos in TestArray)
				{
					if(!GBackSolid(GetPartX(i)-GetX()+xdir*pos[0], GetPartY(i)-GetY()+ydir*pos[1]))
					{
						// Calculate the new position (if we don't move in a direction don't overwrite the old value)
						var new = [0,0];
						if(pos[0])
							new[0] = (GetPartX(i)+xdir*pos[0])*Ladder_Precision-xdir*Ladder_Precision/2;
						else
							new[0] = particles[i][0][0];
						if(pos[1])
							new[1] = (GetPartY(i)+ydir*pos[1])*Ladder_Precision-ydir*Ladder_Precision/2;
						else
							new[1] = particles[i][0][1];
						// Calculate the normalvector to apply the normal force accordingly
						var dif = Vec_Sub(new, particles[i][0]);
						var vel = Vec_Sub(particles[i][0], particles[i][1]);
						var tang = [dif[1], -dif[0]];
						var speed = Vec_Length(vel);
						var normalforce = Vec_Length(dif);
						// if the force is smaller then the speed decelerate
						if(normalforce < speed)
						{
							vel = Vec_Mul(vel, 1000-normalforce*1000/speed);
							vel = Vec_Div(vel, 1000);
							particles[i][1] = Vec_Sub(new, vel);
						}
						// If not stop instantly
						else
							particles[i][1] = new;
						particles[i][0] = new;
						break;
					}
				}
			}
		}
	}
}

func LogArray()
{
	// Helperfunction which has created "TestArray"
	var array = [];
	for(var dist = 1; dist < 10; dist++)
		for(var x = 0; x < 10; x++)
			for(var y = 0; y < 10; y++)
			{
				if(Distance(0,0,x,y) != dist) continue;
				array[GetLength(array)] = [x,y];
			}
	Log("%v", array);
}

func TimeStep() {
	Verlet();
	SatisfyConstraints();
	UpdateLines();
}

// Some vector math
func Vec_Sub(array x, array y) { return [x[0]-y[0], x[1]-y[1]]; }
func Vec_Add(array x, array y) { return [x[0]+y[0], x[1]+y[1]]; }
func Vec_Mul(array x, int   i) { return [x[0]*i,    x[1]*i];    }
func Vec_Div(array x, int   i) { return [x[0]/i,    x[1]/i];    }
func Vec_Dot(array x, array y) { return x[0]*y[0]+x[1]*y[1];    }
func Vec_Length(array x) { return Sqrt(x[0]*x[0]+x[1]*x[1]); }
func Vec_Angle(array x, array y) { return Angle(x[0], x[1], y[0], y[1]); }

func Definition(def) {
	SetProperty("ActMap", {

Hanging = {
	Prototype = Action,
	Name = "Hanging"
},}, def);
	SetProperty("Name", "$Name$", def);
}