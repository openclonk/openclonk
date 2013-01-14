/*
	Grapple Rope
	Author: Randrian

	The rope used for grappling devices.
	Connect(obj1, obj2) connects two objects
	BreakRope() breaks the rope
	DrawIn() draws the hook in
*/

#include Library_Rope

//static const Rope_Iterations = 10;
//static const Rope_Precision = 100;
//static const Rope_PointDistance = 10;

static const Weight = 1;

// Call this to break the rope.
public func BreakRope()
{
	if(length == -1) return;
	length = -1;
	var act1 = objects[0][0];
	var act2 = objects[1][0];
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
	segment = CreateObject(GrappleRope);
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

public func GetConnectStatus() { return !length_auto; }

local HoockAnchored;
local DrawingIn;

/* Callback form the hook, when it hits ground */
public func HockAnchored(bool pull)
{
	HoockAnchored = 1;
}

//func LengthAutoTryCount() { if(!HoockAnchored && !DrawingIn) return 10; return 5; }

public func HookRemoved()
{
	var new_hook = CreateObject(GrappleHook);
	new_hook->New(objects[1][0]->Contained(), this);
	objects[1][0]->SetHook(new_hook);
	objects[0][0] = new_hook;
	DrawIn();
}

/* Callback form the rope library */
public func MaxLengthReached()
{
	var clonk = objects[1][0];
	if(clonk->Contained()) clonk = clonk->Contained();
	if(!HoockAnchored)
	{
		for(var i = 0; i < ParticleCount; i++)
			particles[i][1] = particles[i][0][:];
		DrawIn(1);
	}
}

/* for swinging */
func DoSpeed(int value)
{
	var speed = particles[-1][0][0]-particles[-1][1][0];
	if(speed*value > 0) value += speed/10;
	particles[-1][1][0] -= value;
}

func FxIntHangTimer() { TimeStep(); }

func FxDrawInTimer()
{
	if(length < 15)
	{
		BreakRope();
		return -1;
	}
	DoLength(-5);
	if(!HoockAnchored)
	{
		for(var i = 0; i < ParticleCount; i++)
			particles[i][1] = particles[i][0][:];//Vec_Add(particles[i][0],Vec_Div(Vec_Sub(particles[i][0],particles[i][1]), 2));
		DoLength(-3);
	}
}

func DrawIn(fNoControl)
{
	DrawingIn = 1;
	if(!GetEffect("DrawIn", this))
	{
		AddEffect("DrawIn", this, 1, 1, this);
		SetFixed(0, 1);
		objects[0][0]->SetXDir();
		objects[0][0]->SetYDir();
		ConnectPull();
		var clonk = objects[1][0];
		if(clonk->Contained()) clonk = clonk->Contained();
		if(!fNoControl) RemoveEffect("IntGrappleControl", clonk);
	}
}

func AdjustClonkMovement()
{
	var clonk = objects[1][0];
	if(clonk->Contained()) clonk = clonk->Contained();
	
	var rope_vector = Vec_Sub(particles[-1][0], particles[-2][0]);
	var clonk_speed = [clonk->GetXDir(Rope_Precision), clonk->GetYDir(Rope_Precision)];


	var rope_orthogonal = [rope_vector[1], -rope_vector[0]];
	var new_speed = Vec_Dot(rope_orthogonal, clonk_speed)/Vec_Length(rope_orthogonal);
	
	var clonk_newspeed = Vec_Normalize(rope_orthogonal, new_speed);

	clonk->SetXDir(clonk_newspeed[0], Rope_Precision);
	clonk->SetYDir(clonk_newspeed[1], Rope_Precision);
}

local last_point;

func UpdateLines()
{
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
		}
		
		if(i == 2)
		{
			angle = Angle(particles[2][0][0], particles[2][0][1], particles[0][0][0], particles[0][0][1]);
			start = particles[0][0][:];
			start[0] += -Sin(angle, 45*Rope_Precision/10);
			start[1] += +Cos(angle, 45*Rope_Precision/10);
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
	//		objects[1][0]->Message("%d", Vec_Length(diff)*1000/Rope_Precision/10);
			diffangle = Vec_Angle(diff, [0,0]);
			point = Vec_Add(start, Vec_Div(diff, 2));
			//length = o_length;
			last_point = point;
		}

		if(i == 1)
		{
			segments[i]->SetGraphics(nil, GrappleHook);
			segments[i].MeshTransformation = Trans_Mul(Trans_Translate(1500,0,0),Trans_Scale(1500));
			point[0] += -Cos(diffangle, 15*Rope_Precision/10)+Sin(diffangle, 4*Rope_Precision);
			point[1] += -Cos(diffangle, 4*Rope_Precision)-Sin(diffangle, 15*Rope_Precision/10);
			length = 1000;
		}

		SetLineTransform(segments[i], -diffangle, point[0]*10-GetPartX(i)*1000,point[1]*10-GetPartY(i)*1000, length );

		// Remember the angle
		oldangle = angle;
	}
}

func GetClonkAngle()
{
	if(ParticleCount > 3)
	return Angle(particles[-2][0][0], particles[-2][0][1], particles[-3][0][0], particles[-3][0][1]);
}

local ClonkOldSpeed;

func GetClonkPos()
{
	return particles[-1][0];
}

func GetClonkOff()
{
	return Vec_Sub(particles[-1][0],last_point);
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
	if(!length) return;

	var redo = LengthAutoTryCount();
	while(length_auto && redo)
	{
		var speed = Vec_Length(Vec_Sub(particles[-1][0], particles[-1][1]));
		if(length == GetMaxLength())
		{
			if(ObjContact(objects[1][0]))
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
		if(i == 1) j = ParticleCount-1;
		var obj = objects[i][0];

		if(obj == nil || objects[i][1] == 0) continue;

		if(obj->Contained()) obj = obj->Contained();

		if( (obj->GetAction() == "Walk" || obj->GetAction() == "Scale" || obj->GetAction() == "Hangle"))
			obj->SetAction("Jump");
		if( obj->GetAction() == "Climb")
			obj->SetAction("Jump");

		var xdir = BoundBy(particles[j][0][0]-particles[j][1][0], -300, 300);
		var ydir = BoundBy(particles[j][0][1]-particles[j][1][1], -300, 300);

		obj->SetXDir( xdir, Rope_Precision);
		obj->SetYDir( ydir, Rope_Precision);
	}
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
