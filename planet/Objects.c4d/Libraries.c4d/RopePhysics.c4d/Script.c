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
local Max_Length2;

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
	particles[0] = [[ GetX()*Rope_Precision, GetY()*Rope_Precision],  [(GetX()+1)*Rope_Precision, GetY()*Rope_Precision], [0,1*Rope_Precision], 0];
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

func SetMaxLength(int newlength)
{
	var table = [5,6,7,7,8,9,10,11,12,13,13,14,15,16,17,18,18,19,20,21,22,22,23,24,25,26,26,27,28,29,30,30,31,32,33,33,34,35,36,37,37,38,39,40,40,41,42,43,43,44,45,46,46,47,48,49,49,50,51,52,52,53,54,55,55,56,57,58,58,59,60,61,61,62,63,64,64,65,66,67,67,68,69,69,70,71,72,72,73,74,75,75,76,77,77,78,79,80,80,81,82,83,83,84,85,85,86,87,88,88,89,90,90,91,92,93,93,94,95,95,96,97,98,98,99,100,100,101,102,103,103,104,105,105,106,107,108,108,109,110];
	Max_Length = newlength;
	Max_Length2 = table[newlength];
}

func GetMaxLength()
{
	if(length_auto) return Max_Length;//2;
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
	if(length_auto == 1 && 0)
	{
		var table = [5,6,7,7,8,9,10,11,12,13,13,14,15,16,17,18,18,19,20,21,22,22,23,24,25,26,26,27,28,29,30,30,31,32,33,33,34,35,36,37,37,38,39,40,40,41,42,43,43,44,45,46,46,47,48,49,49,50,51,52,52,53,54,55,55,56,57,58,58,59,60,61,61,62,63,64,64,65,66,67,67,68,69,69,70,71,72,72,73,74,75,75,76,77,77,78,79,80,80,81,82,83,83,84,85,85,86,87,88,88,89,90,90,91,92,93,93,94,95,95,96,97,98,98,99,100,100,101,102,103,103,104,105,105,106,107,108,108,109,110];
		if(!table2 || 1) table2 = [159,233,311,392,475,560,648,737,828,920,1014,1109,1206,1303,1402,1501,1602,1703,1806,1909,2014,2119,2224,2331,2438,2546,2655,2764,2874,2985,3096,3208,3320,3433,3547,3661,3775,3890,4006,4122,4239,4356,4473,4592,4710,4829,4948,5068,5188,5309,5430,5551,5673,5795,5918,6041,6164,6288,6412,6537,6662,6787,6912,7038,7164,7291,7418,7545,7672,7800,7928,8056,8185,8314,8444,8573,8703,8833,8964,9094,9226,9357,9488,9620,9752,9885,10018,10150,10284,10417,10551,10685,10819,10953,11088,11223,11358,11494,11629,11765,11901,12038,12174,12311,12448,12585,12723,12861,12999,13137,13275,13414,13552,13691,13831,13970,14110,14250,14390,14530,14670,14811,14952,15093,15234,15376,15517,15659,15801,15943,16086,16228,16371,16514,16657,16800,16944,17088,17231,17375];

		var newlength = 0;
		while( newlength < GetLength(table) && table[newlength] < length)  newlength++;

		var newlength2 = 0;
		var line_length = GetLineLength();
		while( newlength2 < GetLength(table2) && table2[newlength2] < line_length)  newlength2++;
//		Log("%d %d %d", newlength, length, newlength2);
//		Log("%d %d -> %d", GetLineLength(), length*Rope_Precision, GetLineLength()/Rope_Precision-length);
		length_auto = 0;
		var i = newlength2 - length +1;
		if(i < 0) i = 0;
		while(i--)
		{
			DoLength(+1);
//			Verlet();
//			SatisfyConstraints();
//			LogSpeed();
		}
		/*
		var tabel = [80,126,177,232,292,355,421,491,563,638,716,796,878,963,1050,1139,1230,1323,1417,1514,1612,1712,1814,1917,2022,2128,2236,2346,2457,2569,2683,2798,2914,3032,3151,3271,3393,3516,3640,3765,3891,4019,4148,4278,4409,4541,4674,4808,4943,5080,5217,5356,5495,5635,5777,5919,6063,6207,6352,6498,6645,6794,6943,7092,7243,7395,7547,7701,7855,8010,8166,8323,8481,8639,8798,8958,9119,9281,9444,9607,9771,9936,10102,10268,10435,10603,10772,10941,11111,11282,11454,11626,11799,11973,12148,12323,12499,12675,12853,13031,13209,13389,13569,13749,13931,14113,14295,14479,14663,14847,15033,15219,15405,15593,15780,15969,16158,16348,16538,16729,16921,17113,17306,17499,17693,17888,18083,18279,18475,18672,18870,19068,19267,19466,19666,19867,20000,20000,20000,2000];
		var newlength = 0;
		var line_length = GetLineLength();
		while( newlength < GetLength(tabel) && tabel[newlength] < line_length)  newlength++;
		Log("%d %d", newlength, length);
		Log("%d %d -> %d", GetLineLength(), length*Rope_Precision, GetLineLength()/Rope_Precision-length);
		if(newlength-length)
			DoLength(newlength-length);*/
		for(var i = 0; i < 1; i++)
		{
			Verlet();
			SatisfyConstraints();
//			LogSpeed();
		}
//		AccumulateForces();
	}
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
	particles[ParticleCount] = [[ oldx+xoffset, oldy+yoffset], [ oldx, oldy], [0,1*Rope_Precision], 1]; // Pos, Oldpos, acceleration (gravity), mass

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
		}//Log("DoLength(%d) | %d", dolength, length);
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
/* for the graphics appeareance should be overloaded */
private func UpdateSegmentOverlays() { }
/** Shall display the rope (e.g. rotate the semgents to fit the rope), called every frame
* Should be \b overloaded by the object.
*/
private func UpdateLines() {}

/** The procedure of a time step.
* Should be \b called with a timercall or an effect!
*/
local called3;
local pause_timer;
/* The procedure of a time step, this should be called with a timercall or an effect! */
public func TimeStep()
{//if(!length_auto) Log("%d %d", GetLineLength(), length*Rope_Precision);
	var line_length = GetLineLength();
	var percent = line_length*100/(length*Rope_Precision);
	var per_segment_length = 0;
	if(ParticleCount != 9)
		per_segment_length = (percent-100)/(ParticleCount-9);
//	Log("%d %d", line_length, length*Rope_Precision);
//	Message("%d|%d|%d|%d", length, line_length, percent, per_segment_length);
//	if(pause_timer)
//		return pause_timer--;
	called3+=Rope_Iterations;
//	if(length_auto || 1)	AccumulateForces();
	Verlet();
	SatisfyConstraints();
	ForcesOnObjects();
	UpdateLines();
}

/** Summs all the fores on the segments
* These are gravity and for connect \a loose mode this is also a straightening of the rope.
* Only called when in connect \a loose mode.
*/
func AccumulateForces()
{
	for(var i = 1; i < ParticleCount; i++)
	{
		var fx = 0, fy = 0, angle;
		if(i < ParticleCount-2 && 0)// && length_auto)
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

/** Verlet integration step
* Moves the particles according to their old position and thus speed.
*/
private func Verlet()
{
	var maxspeed = 0;

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
		maxspeed = Max(maxspeed, Distance(x[0], x[1], oldx[0], oldx[1]));
		particles[i][0][0] += x[0]-oldx[0]+a[0];
		particles[i][0][1] += x[1]-oldx[1]+a[1];
		particles[i][1] = temp;
	}
	if(maxspeed < 50) pause_timer = 36;
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

	particles[index][1][0] = particles[index][0][0];
	particles[index][1][1] = particles[index][0][1];
}

public func ConstraintObjects(j)
{
		if(length_auto && length < GetMaxLength())
		{
			// Copy Position of the objects
			for(var i = 0, i2 = 0; i < 2; i++ || i2--)
				SetParticleToObject(i2, i);
		}
}

local called;

public func ConstraintLength2(j)
{
	if(!called) called = 1;
	else called++;
// Satisfy all stick constraints (move the particles to fit the length)
	var normal_restlength = Rope_SegmentLength*Rope_Precision;
	var restlength;
	var x1, x2, invmass1, invmass2;
	var delta, deltalength, diff;
	for(var i=0; i < ParticleCount-1; i++)
	{
		// Keep length
		restlength = normal_restlength; // normal length between two points
		//var restlength = Rope_PointDistance*Rope_Precision; // normal length between laddersegments
		if(i == ParticleCount-2)
		{
			restlength = GetLastLength();
		}
		// Get coordinates and inverse masses
		x1 = particles[i][0];
		x2 = particles[i+1][0];
		invmass1 = particles[i][3];
		invmass2 = particles[i+1][3];
		// calculate difference
		delta = Vec_Sub(x2,x1);
		deltalength = Vec_Length(delta);//Sqrt(Vec_Dot(delta,delta));
		if(deltalength < restlength)
		{
			if(deltalength < restlength*3/4 && length_auto && i < ParticleCount-3)
				;//segment_pick = i;
			continue;
		}
		diff = 0;
		if(deltalength != 0) // savety against division throught zero
			diff = (deltalength-restlength)*1000/(deltalength*(invmass1+invmass2));
		// Set new positions
		particles[i][0]   = Vec_Add(x1, Vec_Div(Vec_Mul(delta, invmass1*diff), 1000));
		particles[i+1][0] = Vec_Sub(x2, Vec_Div(Vec_Mul(delta, invmass2*diff), 1000));
	}
//		if(segment_pick != nil)
//			;//PickSegment(segment_pick);
}

public func ConstraintLength(j)
{
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
		var deltalength = Vec_Length(delta);//Sqrt(Vec_Dot(delta,delta));
		if(deltalength < restlength)
		{
			if(deltalength < restlength*3/4 && length_auto && i < ParticleCount-3)
				;//segment_pick = i;
			continue;
		}
		var diff = 0;
		if(deltalength != 0) // savety against division throught zero
			diff = (deltalength-restlength)*1000/(deltalength*(invmass1+invmass2));
		// Set new positions
		particles[i][0]   = Vec_Add(x1, Vec_Div(Vec_Mul(delta, invmass1*diff), 1000));
		particles[i+1][0] = Vec_Sub(x2, Vec_Div(Vec_Mul(delta, invmass2*diff), 1000));
	}
//		if(segment_pick != nil)
//			;//PickSegment(segment_pick);
}

public func ConstraintLandscape(j)
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

/** Satisfying the constraints for the particles
* The constraints are: Staying at the position of the objects, respecting the length to the next particles and staying out of material
*/
private func SatisfyConstraints(iterations)
{
	var segment_pick = nil;
	var old_particles;
	if(!iterations) iterations = Rope_Iterations;
	for(var j=0; j < iterations; j++)
	{
		ConstraintObjects(j);

		old_particles = particles;
		ConstraintLength(j);
		particles = old_particles;
		ConstraintLength2(j);

		ConstraintLandscape(j);

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

/** Applies the forces on the objects (only non-fixed ones) or adjust the length then the object pulls
*/
local called2;
func ForcesOnObjects()
{
	if(!length) return;
	var length_vertex = 0;
	var redo = 5;
	for(var i=1; i < ParticleCount; i++)
		length_vertex += Distance(particles[i][0][0], particles[i][0][1], particles[i-1][0][0], particles[i-1][0][1]);


/*	while(length_auto && redo)
	{
		var tabel = [80,126,177,232,292,355,421,491,563,638,716,796,878,963,1050,1139,1230,1323,1417,1514,1612,1712,1814,1917,2022,2128,2236,2346,2457,2569,2683,2798,2914,3032,3151,3271,3393,3516,3640,3765,3891,4019,4148,4278,4409,4541,4674,4808,4943,5080,5217,5356,5495,5635,5777,5919,6063,6207,6352,6498,6645,6794,6943,7092,7243,7395,7547,7701,7855,8010,8166,8323,8481,8639,8798,8958,9119,9281,9444,9607,9771,9936,10102,10268,10435,10603,10772,10941,11111,11282,11454,11626,11799,11973,12148,12323,12499,12675,12853,13031,13209,13389,13569,13749,13931,14113,14295,14479,14663,14847,15033,15219,15405,15593,15780,15969,16158,16348,16538,16729,16921,17113,17306,17499,17693,17888,18083,18279,18475,18672,18870,19068,19267,19466,19666,19867,20000,20000,20000,2000];
		var line_length = GetLineLength();
		var change = 0;
		objects[1][0]->Message("%d %d", line_length, tabel[length]);
		if(line_length < tabel[length])
			change = -1;
		else if(line_length > tabel[length+1])
			change = +1;
		if(ParticleCount < 3) change = +1;
		if(change)
		{
			DoLength(change);
		}
		redo = 0;
	}*/
  var angle = Vec_Angle(Vec_Sub(particles[-1][0], particles[-1][1]), [0,0]);
	var angledist = Abs(angle-180);
	while( length_auto && redo)
	{
		angle = Vec_Angle(Vec_Sub(particles[-1][0], particles[-1][1]), [0,0]);
		var speed = Vec_Length(Vec_Sub(particles[-1][0], particles[-1][1]));
		if(length == GetMaxLength())
		{
			if(ObjContact(objects[1][0]))
				speed = 40;
			else speed = 100;
		}
		if(speed > 150) DoLength(1);
		else if(speed < 50 && 1) DoLength(-1); // TODO not just obj 1
		else redo = 0;
		if(redo) redo --;
//		particles[-1][3] = 1;
		var iterations = 0;
		called2 += iterations;
//		SatisfyConstraints(iterations);
	}
//	objects[1][0]->Message("%d %d|%d %d", length_vertex/100, length, ObjContact(objects[1][0]), angledist);
	var j = 0;
	if(!length_auto || length == GetMaxLength() )
	for(var i = 0; i < 2; i++)
	{
		if(i == 1) j = ParticleCount-1;
		var obj = objects[i][0];

		if(obj == nil || objects[i][1] == 0) continue;

		if(obj->Contained()) obj = obj->Contained();
		var speed = obj->GetXDir(Rope_Precision);
		var x = obj->GetX(Rope_Precision), y = obj->GetY(Rope_Precision);
		var xdir = particles[j][0][0]-particles[j][1][0];
		var ydir = particles[j][0][1]-particles[j][1][1];
		obj->SetPosition(particles[j][0][0], particles[j][0][1], 1, Rope_Precision);
		if(obj->Stuck())
			obj->SetPosition(x, y, 1, Rope_Precision);

		
		if(obj->GetComDir() == COMD_Down && ydir > 0)
			obj->SetComDir(COMD_Up);
		if( (obj->GetAction() == "Walk" || obj->GetAction() == "Scale" || obj->GetAction() == "Hangle")) // && !obj->GetContact(-1))
			obj->SetAction("Jump");
		if( obj->GetAction() == "Climb")
			obj->SetAction("Jump");

		obj->SetXDir( particles[j][0][0]-particles[j][1][0], Rope_Precision);
		obj->SetYDir( particles[j][0][1]-particles[j][1][1], Rope_Precision);
	}
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
	for(var dist = 1; dist < 10; dist++)
		for(var x = 0; x < 10; x++)
			for(var y = 0; y < 10; y++)
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
		var x = particles[i][0];
		var oldx = particles[i][1];
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
