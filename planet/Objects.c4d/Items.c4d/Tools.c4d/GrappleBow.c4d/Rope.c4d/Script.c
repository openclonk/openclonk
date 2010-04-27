/*
	Grapple Rope
	Author: Maikel	

	The rope used for grappling devices.
	Call BreakRope() to snap the rope.
	Calls "OnRopeBreak" in both action targets when the rope snaps.
	Rope snaps on overtension and bending (>5 vertices).
	TODO: mix both, breaking should depend on a combination of both tension and bending.
*/

static const Rope_Iterations = 10;
static const Rope_Precision = 100;
static const Rope_PointDistance = 10;

static const Weight = 1;

local particles;
local segments;
local TestArray;

local objects;

local point_count;

local length;

local wait_pull;
local length_auto;

public func DoLength(int dolength, int force)
{
	length += dolength;
	if(length < Rope_PointDistance*2) length = Rope_PointDistance*2;
//	Log("%d", length);

	var last_length = GetLastLength();
	// Remove Points
	while( last_length < Rope_PointDistance*Rope_Precision/2 && point_count > 2)
	{
		point_count--;
		SetLength(particles, point_count);
		segments[point_count]->RemoveObject();
		SetLength(segments, point_count);
		
		last_length = GetLastLength();
	}
	while( last_length > Rope_PointDistance*Rope_Precision*3/2)
	{if(force) force = 0;
		point_count++;
//		Log("%v", particles);
		SetLength(particles, point_count);
		var x2 = particles[point_count-2][0][0];
		var y2 = particles[point_count-2][0][1];

		var x4 = particles[point_count-2][1][0];
		var y4 = particles[point_count-2][1][1];
		particles[point_count-1] = [[ x2, y2], [ x4, y4], [0,2*Rope_Precision], !length_auto*Weight]; // Pos, Oldpos, acceleration (gravity), mass
		
		for(var i = point_count-2; i > 0; i--)
		{
			var x =  particles[i-1][0][0];
			var y =  particles[i-1][0][1];
			var x2 = particles[i][0][0];
			var y2 = particles[i][0][1];
		
			var x3 = particles[i-1][1][0];
			var y3 = particles[i-1][1][1];
			var x4 = particles[i][1][0];
			var y4 = particles[i][1][1];

			particles[i] = [[ x/point_count +x2*(point_count-1)/point_count, y/point_count +y2*(point_count-1)/point_count],
											[ x3/point_count+x4*(point_count-1)/point_count, y3/point_count+y4*(point_count-1)/point_count], [0,2*Rope_Precision], 1];
		}
		SetLength(segments, point_count);
		segments[point_count-1] = CreateObject(GrappleRope);
//		Log("%v", particles);
/*		segments[point_count-1]->SetMaster(this, point_count);
		segments[point_count-1]->SetNextLadder(segments[point_count-2]);
		segments[point_count-2]->SetPreviousLadder(segments[point_count-1]);
		*/
		last_length = GetLastLength();
	}
	return;
}

func GetLastLength()
{
	return length*Rope_Precision-Rope_PointDistance*Rope_Precision*(point_count-1);
}

// Call this to break the rope.
public func BreakRope()
{
	if(length == -1) return;
	length = -1;
	var act1 = objects[0];
	var act2 = objects[1];
	SetAction("Idle");
	// notify action targets.
	if (objects[0] != nil)
		objects[0]->~OnRopeBreak();
	if (objects[1] != nil)
		objects[1]->~OnRopeBreak();
	for(var i = 0; i < point_count; i++)
	{
		segments[i]->RemoveObject();
	}
	RemoveObject();
	return;
}

/*-- Rope connecting --*/

// Connects two objects to the rope, but the length will vary on their positions.
public func Connect(object obj1, object obj2)
{length_auto = 2; wait_pull = 0;
	SetProperty("Visibility", VIS_None);
	length = 30;//ObjectDistance(obj1, obj2)*2;
	objects = [obj1, obj2];
	
	TestArray = [[0, 1], [1, 0], [1, 1], [0, 2], [1, 2], [2, 0], [2, 1], [2, 2], [0, 3], [1, 3], [2, 3], [3, 0], [3, 1], [3, 2], [0, 4], [1, 4], [2, 4], [3, 3], [4, 0], [4, 1], [4, 2], [0, 5], [1, 5], [2, 5], [3, 4], [3, 5], [4, 3], [4, 4], [5, 0], [5, 1], [5, 2], [5, 3], [0, 6], [1, 6], [2, 6], [3, 6], [4, 5], [5, 4], [6, 0], [6, 1], [6, 2], [6, 3], [0, 7], [1, 7], [2, 7], [3, 7], [4, 6], [5, 5], [5, 6], [6, 4], [6, 5], [7, 0], [7, 1], [7, 2], [7, 3], [0, 8], [1, 8], [2, 8], [3, 8], [4, 7], [4, 8], [5, 7], [6, 6], [7, 4], [7, 5], [8, 0], [8, 1], [8, 2], [8, 3], [8, 4], [0, 9], [1, 9], [2, 9], [3, 9], [4, 9], [5, 8], [6, 7], [7, 6], [7, 7], [8, 5], [9, 0], [9, 1], [9, 2], [9, 3], [9, 4]];

	point_count = length/Rope_PointDistance;
	
	segments = CreateArray(point_count);
	for(var i = point_count-1; i >= 0; i--)
	{
		segments[i] = CreateObject(GrappleRope);
//		if(i == 0) segments[i]->SetGraphics("NoRope");
	}
/*	for(var i = 0; i < point_count; i++)
	{
		segments[i]->SetMaster(this, i);
		if(i > 0)
		{
			segments[i]->SetNextLadder(segments[i-1]);
			segments[i-1]->SetPreviousLadder(segments[i]);
		}
	}*/
	particles = CreateArray(point_count);
	var x, y;
	for(var i = 0; i < point_count; i++)
	{
		x = obj1->GetX()*Rope_Precision*(point_count-i)/point_count+obj2->GetX()*Rope_Precision*i/point_count;
		y = obj1->GetY()*Rope_Precision*(point_count-i)/point_count+obj2->GetY()*Rope_Precision*i/point_count;
		particles[i] = [[ x, y], [ x, y], [0,1*Rope_Precision], 1]; // Pos, Oldpos, acceleration (gravity), mass
	}
	particles[0][2] = [0,0];
	particles[0][3] = 0;
	if(length_auto)
		particles[point_count-1][3] = 0;
	SetPosition(GetX(), GetY()-10);
	Verlet(1);
	
	AddEffect("IntHang", this, 1, 1, this);
	return;
}

func AccumulateForces()
{
	var obj = objects[1];
	if(obj->Contained()) obj = obj->Contained();//Log("----------------");
	for(var i = 0; i < point_count; i++)
	{
		var fx = 0, fy = 0, angle;
		if(i < point_count-2)
		{
			angle = Angle(particles[i][0][0], particles[i][0][1], particles[i+1][0][0], particles[i+1][0][1]);
			fx = Sin(angle, 5*Rope_Precision);
			fy =-Cos(angle, 5*Rope_Precision);
//			Log("%d %d - %d°", fx, fy, angle);
		}
		particles[i][2] = [fx,fy+1*Rope_Precision];
	}
}

func CheckLenghtAuto()
{//return;
	if(length_auto == 2) return;
	var obj = objects[1];
	if(obj == nil) return;
	if(obj->Contained()) obj = obj->Contained();
	if(obj->GetAction() != "Jump") ConnectLoose();
	else ConnectPull();
}

func ConnectLoose()
{
//	if(length_auto == 1) return;
	length_auto = 1;
	particles[point_count-1][3] = 0;
}

func ConnectPull()
{
	if(length_auto == 2)
	{
		var obj = objects[1];
		if(obj->Contained()) obj = obj->Contained();
		// Vector r of the last segment
		var r = Vec_Sub(particles[-1][0],particles[-3][0]);
		r = [-r[1], r[0]]; // Get the orthogonal vector
		r = Vec_Normalize(r, 100); // Make it lenght 0
		var v = [obj->GetXDir(Rope_Precision), obj->GetYDir(Rope_Precision)]; // Get the speed vector
		var projection = Vec_Dot(r, Vec_Mul(v, 100))/10000; // Projekt the speed on the orthogonal vector
		obj->SetXDir(r[0]*projection/100, Rope_Precision);
		obj->SetYDir(r[1]*projection/100, Rope_Precision);
	}
//	if(length_auto != 0) wait_pull = 10;;
	length_auto = 0;
	particles[point_count-1][3] = Weight;
}

func FxIntHangTimer() { TimeStep(); }

// Verlet integration step
func Verlet(fFirst)
{
	var fTimeStep = 1;

		var j = 0;
	for(var i = 0; i < 2; i++)
	{
		if(i == 1) j = point_count-1;
		var obj = objects[i];
		if(obj->Contained()) obj = obj->Contained();
		particles[j][0][0] = obj->GetX()*Rope_Precision;
		particles[j][0][1] = obj->GetY()*Rope_Precision;

		particles[j][1][0] = obj->GetX()*Rope_Precision;
		particles[j][1][1] = obj->GetY()*Rope_Precision;
	}

//	particles[point_count-1][0][0] = objects[1]->GetX()*Rope_Precision;
//	particles[point_count-1][0][1] = objects[1]->GetY()*Rope_Precision;
	
	for(var i = 0; i < point_count; i++)
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
}
local length_vertex;

func UpdateLines()
{
	var fTimeStep = 1;
	length_vertex = 0;//Log("----");
	var redo = 0;
	for(var i=0; i < point_count; i++)
	{
		segments[i]->SetPosition(GetPartX(i), GetPartY(i));
//		CreateParticle("Spark", GetPartX(i)-GetX(), GetPartY(i)-GetY(), 0, 0, 10, RGB(255));

		var angle;
		var distance;
		if(i >= 1)
		{
			angle = Angle(particles[i][0][0], particles[i][0][1], particles[i-1][0][0], particles[i-1][0][1]);
			distance = Distance(particles[i][0][0], particles[i][0][1], particles[i-1][0][0], particles[i-1][0][1]);
			length_vertex += distance;
//			Log("%d %d", distance, Rope_PointDistance*Rope_Precision);
		}
		SetSegmentTransform(segments[i], -angle, particles[i][0][0]*10-GetPartX(i)*1000,particles[i][0][1]*10-GetPartY(i)*1000, distance+200);
//		segments[i]->SetR(angle);
	}
//	Log("=== %d %d", length_all, Rope_PointDistance*Rope_Precision*point_count);
	var obj = objects[1];
	if(obj->Contained()) obj = obj->Contained();
	var percent = length_vertex*100/(length*Rope_Precision);
	var normal_percent = length*4/10 + 62; // Adapt the length to the normal bending of the rope (values are from tests ingame)
//	obj->Message("%d %d|%d", percent, normal_percent,point_count);
	if(length_auto && percent > normal_percent) { DoLength(1, 1); redo = 1;}
//	else if(length_auto && Distance(particles[-2][0][0], particles[-2][0][1], particles[-1][0][0], particles[-1][0][1]) <
//		Rope_PointDistance*Rope_Precision*3/4) DoLength(-1);
//	else if(length_auto && Distance(particles[-3][0][0], particles[-3][0][1], particles[-1][0][0], particles[-1][0][1]) <
//		Rope_PointDistance*Rope_Precision/2) DoLength(-1);
//	if(length_auto && length_all < Rope_PointDistance*Rope_Precision*(point_count-1)) DoLength(-1);
	var j = 0;
	if(!length_auto && wait_pull == 0)
	for(var i = 0; i < 2; i++)
	{
		if(i == 1) j = point_count-1;
//		objects[i]->SetPosition(GetPartX(j), GetPartY(j));
		var obj = objects[i];
		if(obj->Contained()) obj = obj->Contained();
		var speed = obj->GetXDir(Rope_Precision);
		obj->SetXDir( obj->GetXDir(Rope_Precision) + (particles[j][0][0]-particles[j][1][0]), Rope_Precision);
//		Log("%d %d", speed, objects[i]->GetXDir(Rope_Precision));
		obj->SetYDir( obj->GetYDir(Rope_Precision) + (particles[j][0][1]-particles[j][1][1]), Rope_Precision);
	}
	if(wait_pull) wait_pull--;
	if(redo) UpdateLines();
}

func SetSegmentTransform(obj, int r, int xoff, int yoff, int length) {
	var fsin=Sin(r, 1000), fcos=Cos(r, 1000); //length = 1200;
	// set matrix values
	obj->SetObjDrawTransform (
		+fcos,             +fsin*length/1200,             xoff, //(1000-fcos)*xoff - fsin*yoff,
		-fsin, +fcos*length/1200, yoff, //(1000-fcos)*yoff + fsin*xoff,
	);
}

func LogSpeed()
{
	// Helperfunction for Debugpurpose
	var array = [];
	for(var i=0; i < point_count; i++)
	{
		var x = particles[i][0];
		var oldx = particles[i][1];
		array[GetLength(array)] = Distance(x[0]-oldx[0], x[1]-oldx[1]);
	}
	Log("%v", array);
}

func GetPartX(index, old) { return (particles[index][old][0]+Rope_Precision/2)/Rope_Precision; }
func GetPartY(index, old) { return (particles[index][old][1]+Rope_Precision/2)/Rope_Precision; }
func GetPartXOffset(index) { return particles[index][0][0]-GetPartX(index)*Rope_Precision; }
func GetPartYOffset(index) { return particles[index][0][1]-GetPartY(index)*Rope_Precision; }

public func OnLadderGrab(clonk, index)
{
	// Do some speed when the clonk jumps on the ladder
	if(index == 0) return;
	particles[index][0][0] += BoundBy(clonk->GetXDir(), -50, 50)*Rope_Precision;
}

public func OnLadderClimb(clonk, index)
{
	// The clonk drags on the upper segments and pushes on the lower ones
	if(index > 2 && index < point_count-3)
	{
		particles[index-2][0][0] -= 1*Rope_Precision*(-1+2*clonk->GetDir());
		particles[index+2][0][0] += 1*Rope_Precision*(-1+2*clonk->GetDir());
	}
	else if(index > 2 && index < point_count-2)
	{
		particles[index-2][0][0] -= 1*Rope_Precision*(-1+2*clonk->GetDir());
		particles[index+1][0][0] += 1*Rope_Precision*(-1+2*clonk->GetDir());
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
	if(index == point_count-1)
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
	for(var j=0; j < Rope_Iterations; j++)
	{
		// Satisfy all stick constraints (move the particles to fit the length)
		for(var i=0; i < point_count-1; i++)
		{
			// Keep length
			var restlength = Rope_PointDistance*Rope_Precision; // normal length between laddersegments
			if(i == point_count-2)
			{
				restlength = GetLastLength();
			}
			// Get coordinates and inverse masses
			var x1 = particles[i][0];
			var x2 = particles[i+1][0];
			var invmass1 = particles[i][3];
			var invmass2 = particles[i+1][3];
/*			if(invmass1 != 0 && invmass2 != 0)
			{
				var tmp = invmass1;
				invmass1 = invmass2;
				invmass2 = tmp;
			}*/
			// calculate difference
			var delta = Vec_Sub(x2,x1);
			var deltalength = Sqrt(Vec_Dot(delta,delta));
			if(deltalength < restlength) continue;
			var diff = (deltalength-restlength)*1000/(deltalength*(invmass1+invmass2));
			// Set new positions
			particles[i][0]   = Vec_Add(x1, Vec_Div(Vec_Mul(delta, invmass1*diff), 1000));
			particles[i+1][0] = Vec_Sub(x2, Vec_Div(Vec_Mul(delta, invmass2*diff), 1000));
		}
		for(var i=0; i < point_count; i++)
		{
			// Don't touch ground
			if(GBackSolid(GetPartX(i)-GetX(), GetPartY(i)-GetY()))
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
//				for(var y = 0; y < 10; y++)
//					for(var x = 0; x < 10; x++)
				for(var pos in TestArray)
				{//var pos = [x,y];
					if(!GBackSolid(GetPartX(i)-GetX()+xdir*pos[0], GetPartY(i)-GetY()+ydir*pos[1]))
					{
						particles[i][0][0] = (GetPartX(i)+xdir*pos[0])*Rope_Precision;
						particles[i][0][1] = (GetPartY(i)+ydir*pos[1])*Rope_Precision;
						break;
					}
				}
			}
		}
	}
/*	objects[0]->SetXDir( GetPartX(i)-GetPartX(i,1) );
	objects[0]->SetYDir(0);
	objects[1]->SetXDir(0);// GetPartX(i)-GetPartX(i,1) );
	objects[1]->SetYDir(0);*/
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
	AccumulateForces();
	CheckLenghtAuto();
	Verlet();
	if(length_auto) DoLength(-1);
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
func Vec_Normalize(array x, int precision) { return Vec_Div(Vec_Mul(x, precision), Vec_Length(x)); }

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