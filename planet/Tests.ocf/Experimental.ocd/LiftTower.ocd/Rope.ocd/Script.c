/*
	Lift Tower Rope
	Author: Randrian, Clonkonaut

	The rope used for the lift tower.
	Connect(obj1, obj2) connects two objects
	BreakRope() breaks the rope
*/

#include Library_Rope

static const Weight = 1;

// Call this to break the rope.
public func BreakRope(bool silent)
{
	if(length == -1) return;
	length = -1;
	var act1 = objects[0][0];
	var act2 = objects[1][0];
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
	if(index == 0) return;
	var segment;
	segment = CreateObject(LiftTower_Rope);
	return segment;
}

/*-- Rope connecting --*/

// Connects two objects to the rope, but the length will vary on their positions.
public func Connect(object obj1, object obj2)
{
	StartRopeConnect(obj1, obj2);
	SetMaxLength(200);
	SetFixed(true, false);

	SetAction("Hide");
	
	AddEffect("IntHang", this, 1, 1, this);
	return;
}

public func GetConnectStatus() { return !length_auto; }

public func HookRemoved()
{
	BreakRope();
}

func FxIntHangTimer() { TimeStep(); }

local last_point;

func UpdateLines()
{
	var fTimeStep = 1;
	var oldangle;
	for(var i=1; i < ParticleCount; i++)
	{
		// Update the Position of the Segment
		segments[i]->SetPosition(GetPartX(i), GetPartY(i));

		// Calculate the angle to the previous segment
		var angle = Angle(particles[i][0][0], particles[i][0][1], particles[i-1][0][0], particles[i-1][0][1]);

		// Draw the left line
		var start = particles[i-1][0][:];
		var end   = particles[i][0][:];

		if(i == 1 && ParticleCount > 2)
		{
			angle = Angle(particles[2][0][0], particles[2][0][1], particles[0][0][0], particles[0][0][1]);
			end = particles[0][0][:];
			end[0] += -Sin(angle, 45*Rope_Precision/10);
			end[1] += +Cos(angle, 45*Rope_Precision/10);
			segments[i]->SetGraphics("Invis");
		}
		
		if(i == 2)
		{
			angle = Angle(particles[2][0][0], particles[2][0][1], particles[0][0][0], particles[0][0][1]);
			start = particles[0][0][:];
			start[0] += -Sin(angle, 45*Rope_Precision/10);
			start[1] += +Cos(angle, 45*Rope_Precision/10);
			segments[i]->SetGraphics("Short");
		}
		
		var diff = Vec_Sub(end,start);
		var point = Vec_Add(start, Vec_Div(diff, 2));
		var diffangle = Vec_Angle(diff, [0,0]);
		var length = Vec_Length(diff)*1000/Rope_Precision/10;
	
		if(i ==  ParticleCount-1)
		{
			var old = particles[i-2][0][:];
			var old_diff = Vec_Sub(start,old);
			var o_length = Vec_Length(old_diff)*1000/Rope_Precision/10;
			if(!o_length) diff = old_diff;
			else diff = Vec_Div(Vec_Mul(old_diff, length),o_length);
			diffangle = Vec_Angle(diff, [0,0]);
			point = Vec_Add(start, Vec_Div(diff, 2));
			last_point = point;
		}

		segments[i]->SetGraphics(nil);
		SetLineTransform(segments[i], -diffangle, point[0]*10-GetPartX(i)*1000,point[1]*10-GetPartY(i)*1000, length );

		// Remember the angle
		oldangle = angle;
	}
}

func GetHookAngle()
{
	if(ParticleCount > 3)
	return Angle(particles[-2][0][0], particles[-2][0][1], particles[-3][0][0], particles[-3][0][1])+180;
}
/*
local HookOldSpeed;

func GetHookPos()
{
	return particles[-1][0];
}

func GetHookOff()
{
	return Vec_Sub(particles[-1][0],last_point);
	var hook = objects[0][0];
	if (hook->Contained()) hook = hook->Contained();
	var speed = [hook->GetXDir(Rope_Precision), hook->GetYDir(Rope_Precision)];
	var offset = speed;
	offset[0] = offset[0]*1000/Rope_Precision;
	offset[1] = offset[1]*1000/Rope_Precision;
	if(!HookOldSpeed)
	{
		HookOldSpeed = offset;
	}
	var ret = HookOldSpeed;
	HookOldSpeed = offset;
	return ret;
}*/

func SetLineTransform(obj, int r, int xoff, int yoff, int length, int layer, int MirrorSegments) {
	if(!MirrorSegments) MirrorSegments = 1;
	var fsin=Sin(r, 1000), fcos=Cos(r, 1000);
	// set matrix values
	obj->SetObjDrawTransform (
		+fcos*MirrorSegments, +fsin*length/1000, xoff,
		-fsin*MirrorSegments, +fcos*length/1000, yoff,layer
	);
}

func Definition(def)
{
	def.LineColors = [RGB(66,33,00), RGB(66,33,00)];
}
local ActMap = {
		Hide = {
			Prototype = Action,
			Name = "Hide",
		},
};
local Name = "$Name$";