/**
	Grapple Rope
	The rope used for grappling devices. Connect(obj1, obj2) connects two objects,
	BreakRope() breaks the rope, DrawIn() draws the hook in.
	
	@author Randrian
*/

#include Library_Rope


static const Weight = 1;

// Call this to break the rope.
public func BreakRope()
{
	if(lib_rope_length == -1) return;
	lib_rope_length = -1;
	var act1 = lib_rope_objects[0][0];
	var act2 = lib_rope_objects[1][0];
	SetAction("Idle");
	// notify action targets.
	if (act1 != nil)
		act1->~OnRopeBreak();
	if (act2 != nil)
		act2->~OnRopeBreak();
	RemoveRope();
	RemoveObject();
	return;
}

/* --------------------- Callbacks from the rope ---------------------- */

/* To be overloaded for special segment behaviour */
private func CreateSegment(int index, object previous)
{
	if(index == 0) return;
	var segment;
	segment = CreateObjectAbove(GrappleRope);
	return segment;
}

/*-- Rope connecting --*/

// Connects two objects to the rope, but the length will vary on their positions.
public func Connect(object obj1, object obj2)
{
	StartRopeConnect(obj1, obj2);
	SetMaxLength(100);
	HoockAnchored = 0;

	SetAction("Hide");
	
	AddEffect("IntHang", this, 1, 1, this);
	return;
}

public func GetConnectStatus() { return !lib_rope_length_auto; }

local HoockAnchored;
local DrawingIn;

/* Callback form the hook, when it hits ground */
public func HockAnchored(bool pull)
{
	HoockAnchored = 1;
}

public func HookRemoved()
{
	var new_hook = CreateObjectAbove(GrappleHook);
	new_hook->New(lib_rope_objects[1][0]->Contained(), this);
	lib_rope_objects[1][0]->SetHook(new_hook);
	lib_rope_objects[0][0] = new_hook;
	DrawIn();
}

/* Callback form the rope library */
public func MaxLengthReached()
{
	var clonk = lib_rope_objects[1][0];
	if (clonk->Contained()) 
		clonk = clonk->Contained();
	if (!HoockAnchored)
	{
		for (var i = 0; i < lib_rope_particle_count; i++)
		{
			lib_rope_particles[i].oldx = lib_rope_particles[i].x;
			lib_rope_particles[i].oldy = lib_rope_particles[i].y;
		}
		DrawIn(true);
	}
}

/* for swinging */
public func DoSpeed(int value)
{
	var speed = lib_rope_particles[-1].x - lib_rope_particles[-1].oldx;
	if (speed * value > 0) 
		value += speed / 10;
	lib_rope_particles[-1].oldx -= value;
}

func FxIntHangTimer() { TimeStep(); }

func FxDrawInTimer()
{
	if(lib_rope_length < 15)
	{
		BreakRope();
		return -1;
	}
	DoLength(-5);
	if (!HoockAnchored)
	{
		for (var i = 0; i < lib_rope_particle_count; i++)
		{
			lib_rope_particles[i].oldx = lib_rope_particles[i].x;
			lib_rope_particles[i].oldy = lib_rope_particles[i].y;
		}
		DoLength(-3);
	}
}

func DrawIn(bool no_control)
{
	DrawingIn = 1;
	if (!GetEffect("DrawIn", this))
	{
		AddEffect("DrawIn", this, 1, 1, this);
		SetFixed(0, 1);
		lib_rope_objects[0][0]->SetXDir();
		lib_rope_objects[0][0]->SetYDir();
		ConnectPull();
		var clonk = lib_rope_objects[1][0];
		if (clonk->Contained()) 
			clonk = clonk->Contained();
		if (!no_control) 
		{
			// Make sure to only remove control effects for this rope.
			var control_effect = GetEffect("IntGrappleControl", clonk);
			if (control_effect && control_effect.CommandTarget->GetRope() == this)
				RemoveEffect(nil, clonk, control_effect);
		}
	}
}

func AdjustClonkMovement()
{
	var clonk = lib_rope_objects[1][0];
	if (clonk->Contained()) 
		clonk = clonk->Contained();
	
	var rope_vector = Vec_Sub([lib_rope_particles[-1].x, lib_rope_particles[-1].y], [lib_rope_particles[-2].x, lib_rope_particles[-2].y]);
	var clonk_speed = [clonk->GetXDir(LIB_ROPE_Precision), clonk->GetYDir(LIB_ROPE_Precision)];

	var rope_orthogonal = [rope_vector[1], -rope_vector[0]];
	var new_speed = Vec_Dot(rope_orthogonal, clonk_speed)/Vec_Length(rope_orthogonal);
	
	var clonk_newspeed = Vec_Normalize(rope_orthogonal, new_speed);

	clonk->SetXDir(clonk_newspeed[0], LIB_ROPE_Precision);
	clonk->SetYDir(clonk_newspeed[1], LIB_ROPE_Precision);
}

local last_point;

func UpdateLines()
{
	var oldangle;
	for(var i=1; i < lib_rope_particle_count; i++)
	{
		// Update the Position of the Segment
		lib_rope_segments[i]->SetPosition(GetPartX(i), GetPartY(i));

		// Calculate the angle to the previous segment
		var angle = Angle(lib_rope_particles[i].x, lib_rope_particles[i].y, lib_rope_particles[i-1].x, lib_rope_particles[i-1].y);

		// Draw the left line
		var start = [lib_rope_particles[i-1].x, lib_rope_particles[i-1].y];
		var end   = [lib_rope_particles[i].x, lib_rope_particles[i].y];

		if(i == 1 && lib_rope_particle_count > 2)
		{
			angle = Angle(lib_rope_particles[2].x, lib_rope_particles[2].y, lib_rope_particles[0].x, lib_rope_particles[0].y);
			end = [lib_rope_particles[0].x, lib_rope_particles[0].y];
			end[0] += -Sin(angle, 45*LIB_ROPE_Precision/10);
			end[1] += +Cos(angle, 45*LIB_ROPE_Precision/10);
		}
		
		if(i == 2)
		{
			angle = Angle(lib_rope_particles[2].x, lib_rope_particles[2].y, lib_rope_particles[0].x, lib_rope_particles[0].y);
			start = [lib_rope_particles[0].x, lib_rope_particles[0].y];
			start[0] += -Sin(angle, 45*LIB_ROPE_Precision/10);
			start[1] += +Cos(angle, 45*LIB_ROPE_Precision/10);
		}
		
		var diff = Vec_Sub(end,start);
		var point = Vec_Add(start, Vec_Div(diff, 2));
		var diffangle = Vec_Angle(diff, [0,0]);
		var length = Vec_Length(diff)*1000/LIB_ROPE_Precision/10;
	
		if(i == lib_rope_particle_count-1)
		{
			var old = [lib_rope_particles[i-2].x, lib_rope_particles[i-2].y];
			var old_diff = Vec_Sub(start,old);
			var o_length = Vec_Length(old_diff)*1000/LIB_ROPE_Precision/10;
			if(!o_length) diff = old_diff;
			else diff = Vec_Div(Vec_Mul(old_diff, length),o_length);
			diffangle = Vec_Angle(diff, [0,0]);
			point = Vec_Add(start, Vec_Div(diff, 2));
			last_point = point;
		}

		if(i == 1)
		{
			lib_rope_segments[i]->SetGraphics(nil, GrappleHook);
			lib_rope_segments[i].MeshTransformation = Trans_Mul(Trans_Translate(1500,0,0),Trans_Scale(1500));
			point[0] += -Cos(diffangle, 15*LIB_ROPE_Precision/10)+Sin(diffangle, 4*LIB_ROPE_Precision);
			point[1] += -Cos(diffangle, 4*LIB_ROPE_Precision)-Sin(diffangle, 15*LIB_ROPE_Precision/10);
			length = 1000;
		}

		SetLineTransform(lib_rope_segments[i], -diffangle, point[0]*10-GetPartX(i)*1000,point[1]*10-GetPartY(i)*1000, length );

		// Remember the angle
		oldangle = angle;
	}
}

func GetClonkAngle()
{
	if(lib_rope_particle_count > 3)
	return Angle(lib_rope_particles[-2].x, lib_rope_particles[-2].y, lib_rope_particles[-3].x, lib_rope_particles[-3].y);
}

local ClonkOldSpeed;

func GetClonkPos()
{
	return [lib_rope_particles[-1].x, lib_rope_particles[-1].y];
}

func GetClonkOff()
{
	return Vec_Sub(GetClonkPos(), last_point);
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

/* --------------------- Overloaded from the rope library --------------------- */

func ForcesOnObjects()
{
	if(!lib_rope_length) return;

	var redo = LengthAutoTryCount();
	while(lib_rope_length_auto && redo)
	{
		var speed = Vec_Length(Vec_Sub([lib_rope_particles[-1].x, lib_rope_particles[-1].y], [lib_rope_particles[-1].oldx, lib_rope_particles[-1].oldy]));
		if(lib_rope_length == GetMaxLength())
		{
			if(ObjContact(lib_rope_objects[1][0]))
				speed = 40;
			else speed = 100;
		}
		if(speed > 150) DoLength(1);
		else if(speed < 50) DoLength(-1); // TODO not just obj 1
		else redo = 0;
		if(redo) redo --;
	}
	var j = 0;
	if(PullObjects() )
	for(var i = 0; i < 2; i++)
	{
		if(i == 1) j = lib_rope_particle_count-1;
		var obj = lib_rope_objects[i][0];

		if(obj == nil || lib_rope_objects[i][1] == 0) continue;

		if(obj->Contained()) obj = obj->Contained();

		if( (obj->GetAction() == "Walk" || obj->GetAction() == "Scale" || obj->GetAction() == "Hangle"))
			obj->SetAction("Jump");
		if( obj->GetAction() == "Climb")
			obj->SetAction("Jump");

		var xdir = BoundBy(lib_rope_particles[j].x-lib_rope_particles[j].oldx, -300, 300);
		var ydir = BoundBy(lib_rope_particles[j].y-lib_rope_particles[j].oldy, -300, 300);

		obj->SetXDir( xdir, LIB_ROPE_Precision);
		obj->SetYDir( ydir, LIB_ROPE_Precision);
	}
}

// Only the grappler is stored.
public func SaveScenarioObject() { return false; }

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
