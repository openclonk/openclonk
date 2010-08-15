/*--
	Rope control
	Authors: Randrian

	Containes the basic functionality for ladders.
--*/

static const Rope_MaxParticles = 15;//30;//15*3;
static const Rope_Iterations = 10;
static const Rope_Precision = 100;
static const Rope_SegmentLength = 5;//2;

local particles;
local segments;
local TestArray;
local ParticleCount;

local objects;
local length;

local length_auto;

local Max_Length;

protected func StartRope()
{
	objects = [[this, 0], [nil, nil]];
	TestArray = [[0, 1], [1, 0], [1, 1], [0, 2], [1, 2], [2, 0], [2, 1], [2, 2], [0, 3], [1, 3], [2, 3], [3, 0], [3, 1], [3, 2], [0, 4], [1, 4], [2, 4], [3, 3], [4, 0], [4, 1], [4, 2], [0, 5], [1, 5], [2, 5], [3, 4], [3, 5], [4, 3], [4, 4], [5, 0], [5, 1], [5, 2], [5, 3], [0, 6], [1, 6], [2, 6], [3, 6], [4, 5], [5, 4], [6, 0], [6, 1], [6, 2], [6, 3], [0, 7], [1, 7], [2, 7], [3, 7], [4, 6], [5, 5], [5, 6], [6, 4], [6, 5], [7, 0], [7, 1], [7, 2], [7, 3], [0, 8], [1, 8], [2, 8], [3, 8], [4, 7], [4, 8], [5, 7], [6, 6], [7, 4], [7, 5], [8, 0], [8, 1], [8, 2], [8, 3], [8, 4], [0, 9], [1, 9], [2, 9], [3, 9], [4, 9], [5, 8], [6, 7], [7, 6], [7, 7], [8, 5], [9, 0], [9, 1], [9, 2], [9, 3], [9, 4]];

	length = Rope_SegmentLength;
	
	ParticleCount = 1;
	segments = CreateArray(ParticleCount);
	segments[0] = CreateSegment(0, nil);

	particles = CreateArray(ParticleCount);
	particles[0] = [[ GetX()*Rope_Precision, GetY()*Rope_Precision],  [(GetX()+1)*Rope_Precision, GetY()*Rope_Precision], [0,1*Rope_Precision], 0];
}

public func StartRopeConnect(object obj1, object obj2)
{
	length = ObjectDistance(obj1, obj2);
	objects = [[obj1, 0], [obj2, 1]];
	
	TestArray = [[0, 1], [1, 0], [1, 1], [0, 2], [1, 2], [2, 0], [2, 1], [2, 2], [0, 3], [1, 3], [2, 3], [3, 0], [3, 1], [3, 2], [0, 4], [1, 4], [2, 4], [3, 3], [4, 0], [4, 1], [4, 2], [0, 5], [1, 5], [2, 5], [3, 4], [3, 5], [4, 3], [4, 4], [5, 0], [5, 1], [5, 2], [5, 3], [0, 6], [1, 6], [2, 6], [3, 6], [4, 5], [5, 4], [6, 0], [6, 1], [6, 2], [6, 3], [0, 7], [1, 7], [2, 7], [3, 7], [4, 6], [5, 5], [5, 6], [6, 4], [6, 5], [7, 0], [7, 1], [7, 2], [7, 3], [0, 8], [1, 8], [2, 8], [3, 8], [4, 7], [4, 8], [5, 7], [6, 6], [7, 4], [7, 5], [8, 0], [8, 1], [8, 2], [8, 3], [8, 4], [0, 9], [1, 9], [2, 9], [3, 9], [4, 9], [5, 8], [6, 7], [7, 6], [7, 7], [8, 5], [9, 0], [9, 1], [9, 2], [9, 3], [9, 4]];

	ParticleCount = length/Rope_SegmentLength;

	var yoff = 0;
	if(ParticleCount < 2)
	{
		ParticleCount = 2;
		yoff = 1;
		length = 10;
	}

	segments = CreateArray(ParticleCount);
	for(var i = 0; i < ParticleCount; i++)
	{
		var prev = nil;
		if(i > 0) prev = segments[i-1];
		segments[i] = CreateSegment(i, prev);
	}

	particles = CreateArray(ParticleCount);
	var x, y;
	for(var i = 0; i < ParticleCount; i++)
	{
		x = obj1->GetX(Rope_Precision)*(ParticleCount-i)/ParticleCount+obj2->GetX(Rope_Precision)*i/ParticleCount;
		y = obj1->GetY(Rope_Precision)*(ParticleCount-i)/ParticleCount+obj2->GetY(Rope_Precision)*i/ParticleCount;
		y += yoff*i;
		particles[i] = [[ x, y], [ x, y], [0,1*Rope_Precision], 1]; // Pos, Oldpos, acceleration (gravity), mass
	}
	particles[0][2] = [0,0];
	particles[0][3] = 0;
	particles[-1][2] = [0,0];
	particles[-1][3] = 1;

	ConnectLoose();

	UpdateSegmentOverlays();
	TimeStep();
	
	return;
}

protected func Destruction()
{
	RemoveRope();
}

public func RemoveRope()
{
	if(segments)
	for(var segment in segments)
		DeleteSegment(segment);
}

public func SetFixed(bool fixed_1, bool fixed_2)
{
	objects[0][1] = !fixed_1;
	objects[1][1] = !fixed_2;
	particles[ 0][3] = objects[0][1];
	particles[-1][3] = objects[1][1];
}

func ConnectLoose()
{
	length_auto = 1;
}

func ConnectPull()
{
	length_auto = 0;
}

/* To be overloaded for special segment behaviour */
private func CreateSegment(int index, object previous) { }

private func DeleteSegment(object segment, previous)
{
	if(segment)
		segment->RemoveObject();
}

/* When the last segment is removed */
private func RopeRemoved() { }

/* Adding and removing segments */
public func AddSegment(int xoffset, int yoffset)
{
	segments[ParticleCount] = CreateSegment(ParticleCount, segments[ParticleCount-1]);

	var oldx = particles[ParticleCount-1][0][0];
	var oldy = particles[ParticleCount-1][0][1];
	particles[ParticleCount] = [[ oldx+xoffset, oldy+yoffset], [ oldx, oldy], [0,1*Rope_Precision], 1]; // Pos, Oldpos, acceleration (gravity), mass

	ParticleCount++;

	length += Rope_SegmentLength;
	
	UpdateSegmentOverlays();
}

public func PickSegment(int index) // Removes a segment form the middle
{
	if(index >= ParticleCount-1) return RemoveSegment();

	var previous = nil;
	if(index > 0) previous = segments[index-1];
	DeleteSegment(segments[index], previous);
	
	for(var i = index; i < ParticleCount-1; i++)
	{
		segments[i] = segments[i+1];
		particles[i] = particles[i+1];
	}
	ParticleCount--;

	SetLength(segments, ParticleCount);
	SetLength(particles, ParticleCount);
	UpdateSegmentOverlays();
}

public func RemoveSegment(fNoLengthAdjust)
{
	ParticleCount--;

	var previous = nil;
	if(ParticleCount-1 >= 0) previous = segments[ParticleCount-1];
	DeleteSegment(segments[ParticleCount], previous);

	if(ParticleCount == 0)
	{
		RopeRemoved();
		return;
	}

	if(!fNoLengthAdjust)
		length -= Rope_SegmentLength;

	SetLength(segments, ParticleCount);
	SetLength(particles, ParticleCount);
	UpdateSegmentOverlays();
}

public func MaxLengthReached() { }

public func DoLength(int dolength)
{
	length += dolength;
	if(Max_Length)
		if(length > Max_Length)
		{
			MaxLengthReached();
			length = Max_Length;
		}
	if(length < Rope_SegmentLength*2) length = Rope_SegmentLength*2;

	var last_length = GetLastLength();
	// Remove Points
	while( last_length < Rope_SegmentLength*Rope_Precision/2 && ParticleCount > 2)
	{
		particles[ParticleCount-2] = particles[ParticleCount-1];
		RemoveSegment(1);

		last_length = GetLastLength();
	}
	var i = 0;
	while( last_length > Rope_SegmentLength*Rope_Precision*3/2)
	{
		ParticleCount++;
		SetLength(particles, ParticleCount);
		var x2 = particles[ParticleCount-2][0][0];
		var y2 = particles[ParticleCount-2][0][1];

		var x4 = particles[ParticleCount-2][1][0];
		var y4 = particles[ParticleCount-2][1][1];
		particles[-1] = [[0,0], [0,0], [0,0], 0];
		particles[-1] = [[x2, y2], [x4, y4], [0,2*Rope_Precision], 1];

		for(var i = ParticleCount-2; i > 0; i--)
		{
			var x =  particles[i-1][0][0];
			var y =  particles[i-1][0][1];
			var x2 = particles[i][0][0];
			var y2 = particles[i][0][1];

			var x3 = particles[i-1][1][0];
			var y3 = particles[i-1][1][1];
			var x4 = particles[i][1][0];
			var y4 = particles[i][1][1];

			particles[i] = [[ x/ParticleCount +x2*(ParticleCount-1)/ParticleCount, y/ParticleCount +y2*(ParticleCount-1)/ParticleCount],
											[ x3/ParticleCount+x4*(ParticleCount-1)/ParticleCount, y3/ParticleCount+y4*(ParticleCount-1)/ParticleCount], [0,1*Rope_Precision], 1];
		}
		SetLength(segments, ParticleCount);
		segments[ParticleCount-1] = CreateSegment(ParticleCount, segments[ParticleCount-2]);

		last_length = GetLastLength();
	}
	UpdateLines();
	particles[ 0][3] = objects[0][1];
	particles[-1][3] = objects[1][1];
	return;
}

func GetLastLength()
{
	return length*Rope_Precision-Rope_SegmentLength*Rope_Precision*(ParticleCount-1);
}

/* for the graphics appeareance should be overloaded */
private func UpdateSegmentOverlays() { }
private func UpdateLines() {}

/* The procedure of a time step, this should be called with a timercall or an effect! */
public func TimeStep()
{
	if(length_auto)	AccumulateForces();
	Verlet();
	SatisfyConstraints();
	ForcesOnObjects();
	UpdateLines();
}

func AccumulateForces()
{
	for(var i = 1; i < ParticleCount; i++)
	{
		var fx = 0, fy = 0, angle;
		if(i < ParticleCount-2 && length_auto)
		{
			angle = Angle(particles[i][0][0], particles[i][0][1], particles[i+1][0][0], particles[i+1][0][1]);
			fx = Sin(angle, 5*Rope_Precision);
			fy =-Cos(angle, 5*Rope_Precision);
/*			angle = Angle(particles[i-1][0][0], particles[i-1][0][1], particles[i+1][0][0], particles[i+1][0][1]);
			var middle = Vec_Div(Vec_Add(particles[i-1][0], particles[i+1][0]), 1);
			var diff = Vec_Sub(middle, particles[i][0]);
			var length = Vec_Length(diff);
			fx = Sin(angle, length/2);
			fy =-Cos(angle, length/2);*/
		}
		particles[i][2] = [fx,fy+1*Rope_Precision];
	}
}

// Verlet integration step
private func Verlet(fFirst)
{
	var fTimeStep = 1;

	// Copy Position of the objects
	var j = 0;
	for(var i = 0; i < 2; i++ || j--)
	{
		if(objects[i][1] == 0)
			SetParticleToObject(j, i);
	}

	// Verlet
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
}

public func SetParticleToObject(int index, int obj_index)
{
	var obj = objects[obj_index][0];
	if(obj == nil) return;

	if(obj->Contained()) obj = obj->Contained();
	particles[index][0][0] = obj->GetX(Rope_Precision);
	particles[index][0][1] = obj->GetY(Rope_Precision);

	particles[index][1][0] = particles[index][0][0];
	particles[index][1][1] = particles[index][0][1];
	particles[index][1][0] = obj->GetX(Rope_Precision);
	particles[index][1][1] = obj->GetY(Rope_Precision);
}

private func SatisfyConstraints()
{
	var segment_pick = nil;
	for(var j=0; j < Rope_Iterations; j++)
	{
		if(length_auto)
		{
			// Copy Position of the objects
			for(var i = 0, i2 = 0; i < 2; i++ || i2--)
				SetParticleToObject(i2, i);
		}
		
		// Satisfy all stick constraints (move the particles to fit the length)
		for(var i=0; i < ParticleCount-1; i++)
		{
			// Keep length
			var restlength = Rope_SegmentLength*Rope_Precision; // normal length between two points
			//var restlength = Rope_PointDistance*Rope_Precision; // normal length between laddersegments
			if(i == ParticleCount-2)
			{
				restlength = GetLastLength();
			}
			// Get coordinates and inverse masses
			var x1 = particles[i][0];
			var x2 = particles[i+1][0];
			var invmass1 = particles[i][3];
			var invmass2 = particles[i+1][3];
			// calculate difference
			var delta = Vec_Sub(x2,x1);
			var deltalength = Sqrt(Vec_Dot(delta,delta));
			if(deltalength < restlength)
			{
				if(deltalength < restlength*3/4 && length_auto && i < ParticleCount-3)
					segment_pick = i;
				continue;
			}
			var diff = 0;
			if(deltalength != 0) // savety against division throught zero
				diff = (deltalength-restlength)*1000/(deltalength*(invmass1+invmass2));
			// Set new positions
			particles[i][0]   = Vec_Add(x1, Vec_Div(Vec_Mul(delta, invmass1*diff), 1000));
			particles[i+1][0] = Vec_Sub(x2, Vec_Div(Vec_Mul(delta, invmass2*diff), 1000));
		}
		if(segment_pick != nil)
			;//PickSegment(segment_pick);
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
							new[0] = (GetPartX(i)+xdir*pos[0])*Rope_Precision-xdir*Rope_Precision/2;
						else
							new[0] = particles[i][0][0];
						if(pos[1])
							new[1] = (GetPartY(i)+ydir*pos[1])*Rope_Precision-ydir*Rope_Precision/2;
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

func GetLineLength()
{
	var length_vertex = 0;
	for(var i=1; i < ParticleCount; i++)
		length_vertex += Distance(particles[i][0][0], particles[i][0][1], particles[i-1][0][0], particles[i-1][0][1]);
	return length_vertex;
}

func ForcesOnObjects()
{
	if(!length) return;
	var length_vertex = 0;
	var redo = 5;
	for(var i=1; i < ParticleCount; i++)
		length_vertex += Distance(particles[i][0][0], particles[i][0][1], particles[i-1][0][0], particles[i-1][0][1]);

	while( length_auto && redo)
	{
		var speed = Vec_Length(Vec_Sub(particles[-1][0], particles[-1][1]));
		if(speed > 150) DoLength(1);
		else if(speed < 50) DoLength(-1);
		else redo = 0;
		if(redo) redo --;
		particles[-1][3] = 1;
		SatisfyConstraints();
	}
	var j = 0;
	if(!length_auto || length == Max_Length)
	for(var i = 0; i < 2; i++)
	{
		if(i == 1) j = ParticleCount-1;
		var obj = objects[i][0];
		if(obj == nil || objects[i][1] == 0) continue;
		if(obj->Contained()) obj = obj->Contained();
		var speed = obj->GetXDir(Rope_Precision);
		var x = obj->GetX(Rope_Precision), y = obj->GetY(Rope_Precision);
		obj->SetPosition(particles[j][0][0], particles[j][0][1], 1, Rope_Precision);
		if(obj->Stuck())
			obj->SetPosition(x, y, 1, Rope_Precision);

		obj->SetXDir( particles[j][0][0]-particles[j][1][0], Rope_Precision);
		obj->SetYDir( particles[j][0][1]-particles[j][1][1], Rope_Precision);
	}
}

/* Helperstuff */
private func LogArray()
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

// Some vector math
func Vec_Sub(array x, array y) { return [x[0]-y[0], x[1]-y[1]]; }
func Vec_Add(array x, array y) { return [x[0]+y[0], x[1]+y[1]]; }
func Vec_Mul(array x, int   i) { return [x[0]*i,    x[1]*i];    }
func Vec_Div(array x, int   i) { return [x[0]/i,    x[1]/i];    }
func Vec_Dot(array x, array y) { return x[0]*y[0]+x[1]*y[1];    }
func Vec_Length(array x) { return Sqrt(x[0]*x[0]+x[1]*x[1]); }
func Vec_Angle(array x, array y) { return Angle(x[0], x[1], y[0], y[1]); }
func Vec_Normalize(array x, int precision) { return Vec_Div(Vec_Mul(x, precision), Vec_Length(x)); }

func GetPartX(index) { return (particles[index][0][0]+Rope_Precision/2)/Rope_Precision; }
func GetPartY(index) { return (particles[index][0][1]+Rope_Precision/2)/Rope_Precision; }
