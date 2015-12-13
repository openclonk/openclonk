/**
	Ropebridge
	A bridge consisting of single wooden planks tied together with ropes.

	@author Randrian
*/

#include Library_Rope

static const Ladder_MaxParticles = 15;
static const Ladder_Iterations = 10;
static const Ladder_Precision = 100;
static const Ladder_SegmentLength = 5;

local MirrorSegments;


public func UpdateSegmentOverlays()
{
	for (var i = 1; i < GetLength(lib_rope_segments); i++)
	{
    	lib_rope_segments[i]->CreateDouble();
		lib_rope_segments[i]->SetGraphics("Line", GetID(), 2, 1);
		lib_rope_segments[i]->SetGraphics("Line", GetID(), 3, 1);
    	lib_rope_segments[i].Double->SetGraphics("Line", GetID(), 4, 1);
    	lib_rope_segments[i]->SetGraphics("Line", GetID(), 5, 1);
	    if (i > 1)
	    {
			lib_rope_segments[i].Double->SetGraphics("Line", GetID(), 7, 1);
			lib_rope_segments[i]->SetGraphics("Line", GetID(), 8, 1);
	    }
		lib_rope_segments[i]->SetSolidMask(6,0,7,3,-5,9);
		if(i > 1 && i < GetLength(lib_rope_segments)-1)
   		{
			lib_rope_segments[i].Plank = 1;
			lib_rope_segments[i]->SetGraphics("Segment", GetID(), 6, 1);
      		lib_rope_segments[i]->SetClrModulation(HSL(255,0,128+Random(128)), 6);
    	}
		if(i % 2 == 0)
		{
			var color = RGB(200,200,200);
			lib_rope_segments[i]->SetClrModulation(color, 2);
			lib_rope_segments[i]->SetClrModulation(color, 3);
			lib_rope_segments[i].Double->SetClrModulation(color, 4);
			lib_rope_segments[i]->SetClrModulation(color, 5);
			lib_rope_segments[i].Double->SetClrModulation(color, 7);
			lib_rope_segments[i]->SetClrModulation(color, 8);
    	}
	}
	lib_rope_segments[0]->CreateDouble();
	lib_rope_segments[-1]->CreateDouble();
	lib_rope_segments[1]->SetSolidMask();
	lib_rope_segments[-1]->SetSolidMask();
	lib_rope_segments[1]->SetGraphics(nil, nil,6);
	return;
}

public func MakeBridge(obj1, obj2)
{
	MirrorSegments = 1;
	SetProperty("Collectible", 0);
	SetCategory(C4D_StaticBack);
	StartRopeConnect(obj1, obj2);
	SetFixed(1, 1);
	ConnectPull();
	AddEffect("IntHang", this, 1, 1, this);
	SetAction("Hanging");
	UpdateSegmentOverlays();
}

public func FxIntHangTimer()
{
	TimeStep();
 	for (var i = 1; i < lib_rope_particle_count - 1; i++)
 	{
   		lib_rope_particles[i].accx = 0;
   		lib_rope_particles[i].accy = lib_rope_segments[i]->~GetLoadWeight();
   	}
}

public func SetFragile()
{
	for (var i = 0; i < lib_rope_particle_count; i++)
		lib_rope_segments[i].fragile = 1;
}

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	props->Remove("Category");
	// Save bridge creation on scenario save
	if (lib_rope_objects && lib_rope_objects[0] && lib_rope_objects[1])
	{
		var o1 = lib_rope_objects[0][0], o2 = lib_rope_objects[1][0];
		if (o1 && o2) props->AddCall("Bridge", this, "MakeBridge", o1, o2);
	}
	return true;
}



/*-- Rope Callbacks --*/

/* To be overloaded for special segment behaviour */
private func CreateSegment(int index, object previous)
{
	var segment = CreateObjectAbove(Ropebridge_Segment);
	segment->SetMaster(this);
	return segment;
}

private func DeleteSegment(object segment, previous)
{
	if (segment)
		segment->RemoveObject();
}

/* When the last segment is removed */
private func RopeRemoved()
{
	RemoveObject();
}


/*-- Segment Graphics --*/

public func DrawRopeLine(start, end, i, int index)
{
	var diff = Vec_Sub(end,start);
	var diffangle = Vec_Angle(diff, [0,0]);
	var point = Vec_Add(start, Vec_Div(diff, 2));
	var length = Vec_Length(diff) * 1000 / Ladder_Precision / 8 + 100;

	if (index != 4 && index != 7)
		SetLineTransform(lib_rope_segments[i], -diffangle, point[0] * 10 - lib_rope_particles[i].x * 10, point[1] * 10 - lib_rope_particles[i].y * 10 + 2000, length, index);
	else if (lib_rope_segments[i].Double)
    	SetLineTransform(lib_rope_segments[i].Double, -diffangle, point[0] * 10 - lib_rope_particles[i].x * 10, point[1] * 10 - lib_rope_particles[i].y * 10 + 2000, length, index);
    return;
}

public func DrawRopeLine2(start, end, i, int index)
{
	var diff = Vec_Sub(end,start);
	var diffangle = Vec_Angle(diff, [0, 0]);
	var point = Vec_Add(start, Vec_Div(diff, 2));
	SetLineTransform(lib_rope_segments[i], -diffangle, point[0] * 10 - lib_rope_particles[i].x * 10, point[1] * 10 - lib_rope_particles[i].y * 10 + 2000, 1000, 6);
	return;
}

public func UpdateLines()
{
	var oldangle = Angle(lib_rope_particles[1].x, lib_rope_particles[1].y, lib_rope_particles[0].x, lib_rope_particles[0].y);
	for (var i = 1; i < lib_rope_particle_count; i++)
	{
		// Update the Position of the Segment
		lib_rope_segments[i]->SetPosition(lib_rope_particles[i].x, lib_rope_particles[i].y, 0, LIB_ROPE_Precision);
    	if (lib_rope_segments[i].Double)
			lib_rope_segments[i].Double->SetPosition(lib_rope_particles[i].x, lib_rope_particles[i].y, 0, LIB_ROPE_Precision);

		// Calculate the angle to the previous segment
		var angle = Angle(lib_rope_particles[i].x, lib_rope_particles[i].y, lib_rope_particles[i - 1].x, lib_rope_particles[i - 1].y);

		// Every segment has not its graphics, but the graphics of the previous segment (or achor for the first)
		// Otherwise the drawing order would be wrong an we would get lines over segments
		
		// Draw the segment as an overlay for the following segment (only the last segment has two graphics (its and the previous)
		lib_rope_segments[i]->SetR(90 + angle);
		// Draw the left line
		var start = GetRopeConnectPosition(i, 0, 0, angle, oldangle);
		var end   = GetRopeConnectPosition(i, 0, 1, angle, oldangle);
    	var end1 = start;
    	DrawRopeLine(start, end, i, 2);
    	if (lib_rope_segments[i].Plank)
      		DrawRopeLine2(start, end, i, 6);
    
		// Draw the right line
		var start = GetRopeConnectPosition(i, 1, 0, angle, oldangle);
		var end = GetRopeConnectPosition(i, 1, 1, angle, oldangle);
		var end2 = start;
    	DrawRopeLine(start, end, i, 3);
    	
		// Draw the upper left line
		var start = GetRopeConnectPosition(i, 0, 0, angle, oldangle);
		var end = GetRopeConnectPosition(i, 0, 1, angle, oldangle);
		var end3 = start[:];
		start[1] -= 800;
		end[1] -= 800;
		DrawRopeLine(start, end, i, 4);
	
		// Draw the upder right line
		var start = GetRopeConnectPosition(i, 1, 0, angle, oldangle);
		var end = GetRopeConnectPosition(i, 1, 1, angle, oldangle);
		var end4 = start[:];
		start[1] -= 800;
		end[1] -= 800;
		DrawRopeLine(start, end, i, 5);
	    
	    if (i > 1)
	    {
			// Draw the upper left line
			var start = end1;
			var end = end3;
			start[1] += 000;
			end[1]-=800;
			DrawRopeLine(start, end, i, 7);
	      
			// Draw the upder right line
			var start = end2;
			var end = end4;
			start[1] += 000;
			end[1] -= 800;
			DrawRopeLine(start, end, i, 8);
	    }
		// Remember the angle.
		oldangle = angle;
	}
}

static const Ropebridge_Segment_LeftXOffset = 200;
static const Ropebridge_Segment_RightXOffset = -100;
static const Ropebridge_Segment_LeftYOffset = 0;
static const Ropebridge_Segment_RightYOffset = -100;
static const Ropebridge_Anchor_RightXOffset = 200;
static const Ropebridge_Anchor_RightYOffset = 0;
static const Ropebridge_Anchor_LeftXOffset = -150;
static const Ropebridge_Anchor_LeftYOffset = -100;

public func GetRopeConnectPosition(int index, int right, int end, int angle, int oldangle)
{
	var SegmentOffset = [[Ropebridge_Segment_LeftXOffset,  Ropebridge_Segment_LeftYOffset],
	                     [Ropebridge_Segment_RightXOffset, Ropebridge_Segment_RightYOffset]];
	var AnchorOffset =  [[Ropebridge_Anchor_LeftXOffset,   Ropebridge_Anchor_LeftYOffset], 
	                     [Ropebridge_Anchor_RightXOffset,  Ropebridge_Anchor_RightYOffset]];
	var point;
	if ((!end && index == 1) || (end && index == lib_rope_particle_count - 1))
	{
		point = [lib_rope_objects[end][0]->GetX(Ladder_Precision), lib_rope_objects[end][0]->GetY(Ladder_Precision)];
		point[0] += -Cos(lib_rope_objects[end][0]->GetR(), AnchorOffset[right][0]*(-1+2*end))+Sin(lib_rope_objects[end][0]->GetR(), AnchorOffset[right][1]);
		point[1] += -Sin(lib_rope_objects[end][0]->GetR(), AnchorOffset[right][0]*(-1+2*end))-Cos(lib_rope_objects[end][0]->GetR(), AnchorOffset[right][1]);
	}
  	else
  	{	
  		if (!end) 
  			index -= 1;
		point = [lib_rope_particles[index].x, lib_rope_particles[index].y];
		point[0] += -Cos(oldangle, SegmentOffset[right][0]*MirrorSegments)-Sin(oldangle, SegmentOffset[right][1]*MirrorSegments);
		point[1] += -Sin(oldangle, SegmentOffset[right][0]*MirrorSegments)+Cos(oldangle, SegmentOffset[right][1]*MirrorSegments);
	}
	return point;
}

public func SetLineTransform(object obj, int r, int xoff, int yoff, int length, int layer, int MirrorSegments) {
	if (!MirrorSegments) 
		MirrorSegments = 1;
	var fsin = Sin(r, 1000), fcos = Cos(r, 1000);
	// Set matrix values.
	obj->SetObjDrawTransform (
		+fcos * MirrorSegments, +fsin * length / 1000, xoff,
		-fsin * MirrorSegments, +fcos * length / 1000, yoff, layer
	);
}

public func Hit()
{
	Sound("Hits::Materials::Wood::WoodHit?");
}

local ActMap = {
	Hanging = {
		Prototype = Action,
		Name = "Hanging",
		Width = 0,
		Height = 0
	},
};
local Name = "$Name$";
local Collectible = 1;
local Rebuy = true;
