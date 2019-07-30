/*
	Lift Tower Rope
	Author: Randrian, Clonkonaut

	The rope used for the lift tower.
	Connect(obj1, obj2) connects two objects
	BreakRope() breaks the rope
*/

#include Library_Rope

static const Library_Rope_MAXLENGTH = 1000;

// Call this to break the rope.
public func BreakRope(bool silent)
{
	if (lib_rope_length == -1) return;
	lib_rope_length = -1;
	var act1 = lib_rope_objects[0][0];
	var act2 = lib_rope_objects[1][0];
	SetAction("Idle");
	// notify action targets.
	if (act1 != nil && !silent)
		act1->~OnRopeBreak();
	if (act2 != nil && !silent)
		act2->~OnRopeBreak();
	RemoveRope();
	RemoveObject();
	return;
}

/* --------------------- Callbacks form the rope ---------------------- */

/* To be overloaded for special segment behaviour */
private func CreateSegment(int index, object previous)
{
	if (index == 0) return;
	var segment;
	segment = CreateObjectAbove(LiftTower_Rope);
	return segment;
}

/*-- Rope connecting --*/

// Connects two objects to the rope, but the length will vary on their positions.
public func Connect(object obj1, object obj2, int max_length)
{
	StartRopeConnect(obj1, obj2);
	if (!max_length) max_length = Library_Rope_MAXLENGTH;
	SetMaxLength(max_length);
	SetFixed(true, false);

	SetAction("Hide");
	
	AddEffect("IntHang", this, 1, 1, this);
	return;
}

public func Reconnect(object reconnect)
{
	lib_rope_objects[1][0] = reconnect;
}

public func GetConnectStatus() { return !lib_rope_length_auto; }

public func HookRemoved()
{
	BreakRope();
}

func FxIntHangTimer() { TimeStep(); }

func UpdateLines()
{
	var oldangle;
	for (var i = 1; i < lib_rope_particle_count; i++)
	{
		// Update the Position of the Segment
		lib_rope_segments[i]->SetPosition(GetPartX(i), GetPartY(i));

		// Calculate the angle to the previous segment
		var angle = Angle(lib_rope_particles[i].x, lib_rope_particles[i].y, lib_rope_particles[i-1].x, lib_rope_particles[i-1].y);

		// Draw the left line
		var start = [lib_rope_particles[i-1].x, lib_rope_particles[i-1].y];
		var end   = [lib_rope_particles[i].x, lib_rope_particles[i].y];

		if (i == 1 && lib_rope_particle_count > 2)
		{
			angle = Angle(lib_rope_particles[2].x, lib_rope_particles[2].y, lib_rope_particles[0].x, lib_rope_particles[0].y);
			end = [lib_rope_particles[0].x, lib_rope_particles[0].y];
			end[0] += -Sin(angle, 45*LIB_ROPE_Precision/10);
			end[1] += +Cos(angle, 45*LIB_ROPE_Precision/10);
			lib_rope_segments[i]->SetGraphics("Invis");
		}
		
		if (i == 2)
		{
			angle = Angle(lib_rope_particles[2].x, lib_rope_particles[2].y, lib_rope_particles[0].x, lib_rope_particles[0].y);
			start = [lib_rope_particles[0].x, lib_rope_particles[0].y];
			start[0] += -Sin(angle, 45*LIB_ROPE_Precision/10);
			start[1] += +Cos(angle, 45*LIB_ROPE_Precision/10);
			lib_rope_segments[i]->SetGraphics("Short");
		}
		
		var diff = Vec_Sub(end, start);
		var point = Vec_Add(start, Vec_Div(diff, 2));
		var diffangle = Vec_Angle(diff, [0, 0]);
		var length = Vec_Length(diff)*1000/LIB_ROPE_Precision/10;
	
		if (i ==  lib_rope_particle_count-1)
		{
			var old = [lib_rope_particles[i-2].x, lib_rope_particles[i-2].y];
			var old_diff = Vec_Sub(start, old);
			var o_length = Vec_Length(old_diff)*1000/LIB_ROPE_Precision/10;
			if (!o_length) diff = old_diff;
			else diff = Vec_Div(Vec_Mul(old_diff, length),o_length);
			diffangle = Vec_Angle(diff, [0, 0]);
			point = Vec_Add(start, Vec_Div(diff, 2));
		}

		lib_rope_segments[i]->SetGraphics(nil);
		SetLineTransform(lib_rope_segments[i], -diffangle, point[0]*10-GetPartX(i)*1000, point[1]*10-GetPartY(i)*1000, length );

		// Remember the angle
		oldangle = angle;
	}
}

public func GetHookAngle()
{
	if (lib_rope_particle_count > 3)
	return Angle(lib_rope_particles[-2].x, lib_rope_particles[-2].y, lib_rope_particles[-3].x, lib_rope_particles[-3].y)+180;
}

public func SetLineTransform(obj, int r, int xoff, int yoff, int length, int layer, int MirrorSegments) {
	if (!MirrorSegments) MirrorSegments = 1;
	var fsin = Sin(r, 1000), fcos = Cos(r, 1000);
	// set matrix values
	obj->SetObjDrawTransform (
		+fcos*MirrorSegments, +fsin*length/1000, xoff,
		-fsin*MirrorSegments, +fcos*length/1000, yoff, layer
	);
}

/* Overload */

local pull_position, pull_faults, pull_frame;

// Altered to not just pull the objects into the rope's direction but
// if the object doesn't not move it is tried to shake it free by applying
// impulses to every direction
func ForcesOnObjects()
{
	if (!lib_rope_length) return;

	var redo = LengthAutoTryCount();
	while (lib_rope_length_auto && redo)
	{
		var speed = Vec_Length(Vec_Sub([lib_rope_particles[-1].x, lib_rope_particles[-1].y], [lib_rope_particles[-1].oldx, lib_rope_particles[-1].oldy]));
		if (lib_rope_length == GetMaxLength())
		{
			if (ObjContact(lib_rope_objects[1][0]))
				speed = 40;
			else speed = 100;
		}
		if (speed > 150) DoLength(1);
		else if (speed < 50) DoLength(-1);
		else redo = 0;
		if (redo) redo --;
	}
	var j = 0;
	if (PullObjects())
	for (var i = 0; i < 2; i++)
	{
		if (i == 1) j = lib_rope_particle_count-1;
		var obj = lib_rope_objects[i][0];

		if (obj == nil || !lib_rope_objects[i][1]) continue;

		if (obj->Contained())
			obj = obj->Contained();

		if (obj->GetAction() == "Walk" || obj->GetAction() == "Scale" || obj->GetAction() == "Hangle" || obj->GetAction() == "Climb")
			obj->SetAction("Jump");

		var xdir = BoundBy(lib_rope_particles[j].x-lib_rope_particles[j].oldx, -100, 100);
		var ydir = lib_rope_particles[j].y-lib_rope_particles[j].oldy;

		if (!obj->GetContact(-1))
			ydir = BoundBy(ydir, -120, 120);

		if (pull_position && pull_frame != FrameCounter() && !Distance(pull_position[0], pull_position[1], obj->GetX(), obj->GetY()))
		{
			if (!pull_faults)
			{
				ydir *= -1;
				pull_faults++;
			}
			else
			{
				xdir *= -1;
				pull_faults = 0;
			}
		}
		else
		{
			pull_position = [obj->GetX(), obj->GetY()];
			pull_faults = 0;
		}
		pull_frame = FrameCounter();

		obj->SetXDir( xdir, LIB_ROPE_Precision);
		obj->SetYDir( obj->GetYDir(LIB_ROPE_Precision) + ydir, LIB_ROPE_Precision);
	}
}

// Altered to function in 'ConnectPull' mode
public func ConstraintObjects()
{
	if (lib_rope_length < GetMaxLength()) // in the rope library this is
	{
		for (var i = 0, i2 = 0; i < 2; i++ || i2--)
			SetParticleToObject(i2, i);
	}
}

// This is called constantly by the lift tower as long as something is reeled in
// Altered so the rope will not get shorter than the distance between the tower
// and the hooked up object
public func DoLength(int dolength)
{
	var obj = lib_rope_objects[0][0]; // First connected object
	var obj2 = lib_rope_objects[1][0]; // Second connected object
	// I guess this is not possible but just to be sure
	if (!obj || !obj2) return _inherited(dolength);
	if (obj->Contained()) obj = obj->Contained();
	if (obj2->Contained()) obj2 = obj2->Contained();

	// Line would be shorter than the distance? Do nothing
	if (dolength < 0 && ObjectDistance(obj, obj2) > GetLineLength()/100) return;
	return _inherited(dolength);
}

func Definition(def)
{
	def.LineColors = [RGB(66, 33, 00), RGB(66, 33, 00)];
}
local ActMap = {
		Hide = {
			Prototype = Action,
			Name = "Hide",
		},
};
local Name = "$Name$";
