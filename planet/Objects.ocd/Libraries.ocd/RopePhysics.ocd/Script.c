/** Rope control
* This library contains all functions for using dynamic ropes
* @sa Ropeladder, GrapplerRope
* @author Randrian
*/

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

func GetRopeGravity()
{
	return GetGravity()*Rope_Precision/100;
}

/** Starts a rope
* The rope object itself is used as first pole where the rope is connected to.
* @param obj1 The first object
* @param obj2 The second object
*/
protected func StartRope()
{
	objects = [[this, 0], [nil, nil]];
	TestArray = [[0, 1], [1, 0], [1, 1], [0, 2], [1, 2], [2, 0], [2, 1], [2, 2], [0, 3], [1, 3], [2, 3], [3, 0], [3, 1], [3, 2], [0, 4], [1, 4], [2, 4], [3, 3], [4, 0], [4, 1], [4, 2], [0, 5], [1, 5], [2, 5], [3, 4], [3, 5], [4, 3], [4, 4], [5, 0], [5, 1], [5, 2], [5, 3], [0, 6], [1, 6], [2, 6], [3, 6], [4, 5], [5, 4], [6, 0], [6, 1], [6, 2], [6, 3], [0, 7], [1, 7], [2, 7], [3, 7], [4, 6], [5, 5], [5, 6], [6, 4], [6, 5], [7, 0], [7, 1], [7, 2], [7, 3], [0, 8], [1, 8], [2, 8], [3, 8], [4, 7], [4, 8], [5, 7], [6, 6], [7, 4], [7, 5], [8, 0], [8, 1], [8, 2], [8, 3], [8, 4], [0, 9], [1, 9], [2, 9], [3, 9], [4, 9], [5, 8], [6, 7], [7, 6], [7, 7], [8, 5], [9, 0], [9, 1], [9, 2], [9, 3], [9, 4]];
	length = Rope_SegmentLength;

	ParticleCount = 1;
	segments = CreateArray(ParticleCount);
	segments[0] = CreateSegment(0, nil);

	particles = CreateArray(ParticleCount);
	particles[0] = [[ GetX()*Rope_Precision, GetY()*Rope_Precision],  [(GetX()+1)*Rope_Precision, GetY()*Rope_Precision], [0,GetRopeGravity()], 0];
}

/** Connects \a obj1 and \a obj2
* Connects the two objects with a rope. Should be only used, when the path between the objects is free, so that the line doesn't go throught material.
* By default the first objects is set to fixed (no force is applied on it) and the second to loose (the line can affect the position or the position can affect the line's length)
* @param obj1 The first object
* @param obj2 The second object
*/
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
		particles[i] = [[ x, y], [ x, y], [0,GetRopeGravity()], 1]; // Pos, Oldpos, acceleration (gravity), mass
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

func SetMaxLength(int newlength)
{
	Max_Length = newlength;
}

func GetMaxLength()
{
	return Max_Length;
}

/** Removes the rope
* All segments are removed. This should be called to clear the rope.
*/
public func RemoveRope()
{
	if(segments)
	for(var segment in segments)
		DeleteSegment(segment);
}

/** Sets the fixed status of the two targets
* When a target is fixed, it only serves as a fixed starting or ending point for the rope. No force is applied on the object.
* For a non fixed target force is applied or in case of a rope that is set to ConnectLoose the strength with wich the object pulls affects the length of the rope.
* @param fixed_1 whether object1 shall be fixed
* @param fixed_2 whether object2 shall be fixed
*/
public func SetFixed(bool fixed_1, bool fixed_2)
{
	objects[0][1] = !fixed_1;
	objects[1][1] = !fixed_2;
	particles[ 0][3] = objects[0][1];
	particles[-1][3] = objects[1][1];
}

/** Sets the rope connection mode to \a loose
* A loose rope will vary the length according to the connected objects. If a object is non-fixed and pulls at the rope the length will increase.
* If it doesn't pull the length will decrease.
*/
func ConnectLoose()
{
	length_auto = 1;
}

/** Sets the rope connection mode to \a pull
* The rope tries to keep its length and pull at non-fixed objects.
*/
local table2 ;//= [80,126,177,232,292,355,421,491,563,638,716,796,878,963,1050,1139,1230,1323,1417,1514,1612,1712,1814,1917,2022,2128,2236,2346,2457,2569,2683,2798,2914,3032,3151,3271,3393,3516,3640,3765,3891,4019,4148,4278,4409,4541,4674,4808,4943,5080,5217,5356,5495,5635,5777,5919,6063,6207,6352,6498,6645,6794,6943,7092,7243,7395,7547,7701,7855,8010,8166,8323,8481,8639,8798,8958,9119,9281,9444,9607,9771,9936,10102,10268,10435,10603,10772,10941,11111,11282,11454,11626,11799,11973,12148,12323,12499,12675,12853,13031,13209,13389,13569,13749,13931,14113,14295,14479,14663,14847,15033,15219,15405,15593,15780,15969,16158,16348,16538,16729,16921,17113,17306,17499,17693,17888,18083,18279,18475,18672,18870,19068,19267,19466,19666,19867,20000,20000,20000,2000];

func ConnectPull()
{
	length_auto = 0;
}

func TogglePull()
{
	if(length_auto) ConnectPull();
	else ConnectLoose();
}

/** Create a new segment
* This function should be \a overloaded so that the rope can create it's own specific objects
* @param index the index of the new segment
* @param previous the previous segment (in case it needs to be notified)
*/
/* To be overloaded for special segment behaviour */
private func CreateSegment(int index, object previous) { }

/** Remove a new segment
* Can be overloaded, when the segments require special deletion behaviour (e.g. notify other segments)
* @param segment the segment to be removed
* @param previous the previous segment (in case it needs to be notified)
*/
private func DeleteSegment(object segment, previous)
{
	if(segment)
		segment->RemoveObject();
}

/** Callback when the rope has lost it's last segment with \c RemoveSegment
*/
/* When the last segment is removed */
private func RopeRemoved() { }

/** Adds a new Segment to the rope and increases the length of the rope
* @param xoffset x offset of the newly inserted segment
* @param yoffset y offset of the newly inserted segment
*/
/* Adding and removing segments */
public func AddSegment(int xoffset, int yoffset)
{
	segments[ParticleCount] = CreateSegment(ParticleCount, segments[ParticleCount-1]);

	var oldx = particles[ParticleCount-1][0][0];
	var oldy = particles[ParticleCount-1][0][1];
	particles[ParticleCount] = [[ oldx+xoffset, oldy+yoffset], [ oldx, oldy], [0,GetRopeGravity()], 1]; // Pos, Oldpos, acceleration (gravity), mass

	ParticleCount++;

	length += Rope_SegmentLength;

	UpdateSegmentOverlays();
}

/** Removes a segment from the middle of the rope
* @param index index of the segment to be removed
*/
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

/** Removes a segment from the end of the rope
* @param fNoLengthAdjust wether the length of the rope shall be decreased accordingly
*/
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

/** Callback, when the maximal length of the rope is reached
*/
public func MaxLengthReached() { }

/** Increases the length of the rope by \a dolength
* Segments are inserted or removed to fit the rope to the new length
* @param dolength the length difference
*/
public func DoLength(int dolength)
{
	length += dolength;
	if(GetMaxLength())
		if(length >= GetMaxLength())
		{
			MaxLengthReached();
			length = GetMaxLength();
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
//		particles[-1] = [[0,0], [0,0], [0,0], 0];
		particles[-1] = [[x2, y2], [x4, y4], [0,GetRopeGravity()], 1];

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

/** Returns the length of the last segment (other segments have \c Rope_SegmentLength length)
* @return the length of the last segment
*/
func GetLastLength()
{
	return length*Rope_Precision-Rope_SegmentLength*Rope_Precision*(ParticleCount-1);
}

/** This is called when a new segment is added, the segments can adjust their appeareance to that
* Should be \b overloaded by the object.
*/
private func UpdateSegmentOverlays() { }
/** Shall display the rope (e.g. rotate the semgents to fit the rope), called every frame
* Should be \b overloaded by the object.
*/
private func UpdateLines() {}

/** The procedure of a time step.
* Should be \b called with a timercall or an effect!
*/
public func TimeStep()
{
	Verlet();
	SatisfyConstraints();
	ForcesOnObjects();
	UpdateLines();
}

/** Summs all the fores on the segments
* These are gravity and for connect \a loose mode this is also a straightening of the rope.
* Only called when in connect \a loose mode. NOT USED AT THE MOMENT!
*/
func AccumulateForces()
{
	for(var i = 1; i < ParticleCount; i++)
	{
		var fx = 0, fy = 0, angle;
		if(i < ParticleCount-2)
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
		particles[i][2] = [fx,fy+GetRopeGravity()];
	}
}

/** Verlet integration step
* Moves the particles according to their old position and thus speed.
*/
private func Verlet()
{
	// Copy Position of the objects
	var j = 0;
	for(var i = 0; i < 2; i++ || j--)
	{//Log("Verlet %d %d (%v), %d", i, j, objects, objects[i][1] == 0);
//		if(objects[i][1] == 0 )//|| PullObjects())
			SetParticleToObject(j, i);
	}//Log("End");

	// Verlet
	var start = 1;
	var last = ParticleCount;
	if(objects[-1][1] == 1 && PullObjects()) last -= 1;
	for(var i = start; i < ParticleCount; i++)
	{
		if(i == ParticleCount-1)
		{
			//particles[i][0] = particles[i][1][:];
			//continue;
		}
		var x = particles[i][0][:];
		var temp = x;
		var oldx = particles[i][1][:];
		var a = particles[i][2][:];

		// Verlet step, get speed out of distance moved relativ to the last position
		particles[i][0][0] += x[0]-oldx[0]+a[0];
		particles[i][0][1] += x[1]-oldx[1]+a[1];
		particles[i][1] = temp;
		particles[i][4] = 0;
	}
}

/** Moves a particle to the position of the object
* @param index the index of the particle to be moved
* @param obj_index the index of the object to be moved
*/
public func SetParticleToObject(int index, int obj_index)
{
	var obj = objects[obj_index][0];
	if(obj == nil) return;

	if(obj->Contained()) obj = obj->Contained();
	particles[index][0][0] = obj->GetX(Rope_Precision);
	particles[index][0][1] = obj->GetY(Rope_Precision);
	return;
//Log("Set %d %d", index, obj_index);
//	particles[index][1][0] = particles[index][0][0];
//	particles[index][1][1] = particles[index][0][1];
}

public func ConstraintObjects()
{
		if(length_auto && length < GetMaxLength())
		{
			// Copy Position of the objects
			for(var i = 0, i2 = 0; i < 2; i++ || i2--)
				SetParticleToObject(i2, i);
		}
}

public func ConstraintLength()
{
	// Satisfy all stick constraints (move the particles to fit the length)
	var normal_restlength = Rope_SegmentLength*Rope_Precision;
	var restlength;
	var invmass1, invmass2;
	var delta = [0,0], deltaDot, deltalength; // diff
	for(var i=0; i < ParticleCount-1; i++)
	{
		// Keep length
		restlength = normal_restlength; // normal length between two points
		if(i == ParticleCount-2)
			restlength = GetLastLength();
		// Get coordinates and inverse masses
		invmass1 = particles[i][3];
		invmass2 = particles[i+1][3];
		// calculate difference
		delta[0] = particles[i+1][0][0]-particles[i][0][0];
		delta[1] = particles[i+1][0][1]-particles[i][0][1];
		deltaDot = delta[0]*delta[0]+delta[1]*delta[1];
		deltalength = Sqrt(delta[0]*delta[0]+delta[1]*delta[1]);
		if(deltalength < restlength)
			continue;
		delta[0] = delta[0]*(deltalength-restlength)/(deltalength*(invmass1+invmass2));
		delta[1] = delta[1]*(deltalength-restlength)/(deltalength*(invmass1+invmass2));
		// Set new positions
		particles[i  ][0][0]   += delta[0]*invmass1;//*invmass1;//*diff/1000;
		particles[i  ][0][1]   += delta[1]*invmass1;//*invmass1;//*diff/1000;
		particles[i+1][0][0]   -= delta[0]*invmass2;//*invmass2;//*diff/1000;
		particles[i+1][0][1]   -= delta[1]*invmass2;//*invmass2;//*diff/1000;
	}
}

public func ConstraintLandscape()
{
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
				var found = 0;
				// Look for all possible places where the particle could move (from nearest to farest)
				for(var pos in TestArray)
				{
					if(!GBackSolid(GetPartX(i)-GetX()+xdir*pos[0], GetPartY(i)-GetY()+ydir*pos[1]))
					{
						// Calculate the new position (if we don't move in a direction don't overwrite the old value)
						var new = [0,0];
						if(pos[0])
							new[0] = (GetPartX(i)+xdir*pos[0])*Rope_Precision-xdir*Rope_Precision/2+xdir;
						else
							new[0] = particles[i][0][0];
						if(pos[1])
							new[1] = (GetPartY(i)+ydir*pos[1])*Rope_Precision-ydir*Rope_Precision/2+ydir;
						else
							new[1] = particles[i][0][1];
						particles[i][4] = 1; // Notifier for applying friction after the constraints
						particles[i][0] = new;
						found = 1;
						break;
					}
				}
				// No possibility to move the particle out? Then reset it. The old position should be valid
				if(!found)
					particles[i][0] = particles[i][1][:];
			}
		}
}

/** Satisfying the constraints for the particles
* The constraints are: Staying at the position of the objects, respecting the length to the next particles and staying out of material
*/
private func SatisfyConstraints()
{
	for(var j=0; j < Rope_Iterations; j++)
	{
		ConstraintObjects();
		ConstraintLength();
		ConstraintLandscape();
	}
	// Apply friction for those how have the notifier for it.
	// Friction just means that the velocity is divided by 2 to simulatie a friction force
	for(var i=0; i < ParticleCount; i++)
	{
		if(!particles[i][4]) continue;
		var newvel = Vec_Sub(particles[i][0], particles[i][1]);
		newvel = Vec_Div(newvel, 2);
		particles[i][1] = Vec_Sub(particles[i][0], newvel);
	}
}

/** Returns the length of the rope
* @return the length of the rope
*/
func GetLineLength()
{
	var length_vertex = 0;
	for(var i=1; i < ParticleCount; i++)
		length_vertex += Distance(particles[i][0][0], particles[i][0][1], particles[i-1][0][0], particles[i-1][0][1]);
	return length_vertex;
}

func LengthAutoTryCount() { return 5; }

/** Applies the forces on the objects (only non-fixed ones) or adjust the length then the object pulls
*/
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

/*		var x = obj->GetX(Rope_Precision), y = obj->GetY(Rope_Precision);
		obj->SetPosition(particles[j][0][0], particles[j][0][1], 1, Rope_Precision);
		if(obj->Stuck())
			obj->SetPosition(x, y, 1, Rope_Precision);*/

		if( (obj->GetAction() == "Walk" || obj->GetAction() == "Scale" || obj->GetAction() == "Hangle"))
			obj->SetAction("Jump");
		if( obj->GetAction() == "Climb")
			obj->SetAction("Jump");

		obj->SetXDir( particles[j][0][0]-particles[j][1][0], Rope_Precision);
		obj->SetYDir( particles[j][0][1]-particles[j][1][1], Rope_Precision);
	}
}

func PullObjects()
{
	return !length_auto || length == GetMaxLength();
}

func ObjContact(obj)
{
	if(obj->Contained()) obj = obj->Contained();
	if(obj->GetContact(-1)) return true;
	return false;
}

/** Just a helperfunction which has creaded the \c TestArray variable to test the landscape to move out particles
*/
/* Helperstuff */
private func LogArray()
{
	// Helperfunction which has created "TestArray"
	var array = [];
	for(var dist = 1; dist < 20; dist++)
		for(var x = 0; x < 20; x++)
			for(var y = 0; y < 20; y++)
			{
				if(Distance(0,0,x,y) != dist) continue;
				array[GetLength(array)] = [x,y];
			}
	Log("%v", array);
}

/** Testfunction which gives the speed of the particles
*/
func LogSpeed()
{
	// Helperfunction for Debugpurpose
	var array = [];
	for(var i=0; i < ParticleCount; i++)
	{
		var x = particles[i][0][:];
		var oldx = particles[i][1][:];
		array[GetLength(array)] = Distance(x[0]-oldx[0], x[1]-oldx[1]);
	}
	Log("%v", array);
}

/** Addition of two vectors
* @param x vector 1
* @param y vector to add
* @param return vector \a x plus \a y
*/
func Vec_Add(array x, array y) { return [x[0]+y[0], x[1]+y[1]]; }

/** Subtraction of two vectors
* @param x vector 1
* @param y vector to subtract
* @param return vector \a x minus \a y
*/
func Vec_Sub(array x, array y) { return [x[0]-y[0], x[1]-y[1]]; }

/** Multiplication of a vector and a number
* @param x vector
* @param i number
* @param return \a i times \a x
*/
func Vec_Mul(array x, int   i) { return [x[0]*i,    x[1]*i];    }

/** Division of a vector and a number
* @param x vector
* @param i number
* @param return \a x divided throught \a i
*/
func Vec_Div(array x, int   i) { return [x[0]/i,    x[1]/i];    }

/** Dot product of two vectors
* @param x vector 1
* @param y vector 2
* @param return dot product between \a x and \a y
*/
func Vec_Dot(array x, array y) { return x[0]*y[0]+x[1]*y[1];    }

/** Length of a vector
* @param x vector
* @param return length of vector \a a
*/
func Vec_Length(array x) { return Sqrt(x[0]*x[0]+x[1]*x[1]); }

/** Angle between two vectors
* @param x vector 1
* @param y vector 2
* @param return angle between \a x and \a y
*/
func Vec_Angle(array x, array y) { return Angle(x[0], x[1], y[0], y[1]); }

/** Normalizes a vector with precision
* @param x vector
* @param precision factor for the resultion length
* @param return the normalize vector with length 1*precision
*/
func Vec_Normalize(array x, int precision) { return Vec_Div(Vec_Mul(x, precision), Vec_Length(x)); }

/** Gives the the rounded x coordinate of particles \a index
* @param index the particle which position is desired
* @param return the x coordinate of particle \a index
*/
func GetPartX(index) { return (particles[index][0][0]+Rope_Precision/2)/Rope_Precision; }

/** Gives the the rounded y coordinate of particles \a index
* @param index the particle which position is desired
* @param return the y coordinate of particle \a index
*/
func GetPartY(index) { return (particles[index][0][1]+Rope_Precision/2)/Rope_Precision; }
