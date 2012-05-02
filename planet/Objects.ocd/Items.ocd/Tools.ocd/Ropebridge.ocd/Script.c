/*
	Ropebridge
	Author: Randrian

	A bridge consisting of single wooden planks tied together with ropes.

*/

#include Library_Rope

static const Ladder_MaxParticles = 15;//30;//15*3;
static const Ladder_Iterations = 10;
static const Ladder_Precision = 100;
static const Ladder_SegmentLength = 5;//2;

local particles;
local segments;

local MirrorSegments;

local ParticleCount;

public func UpdateSegmentOverlays()
{
	for(var i = 1; i < GetLength(segments); i++)
	{
    segments[i]->CreateDouble();
		segments[i]->SetGraphics("Line", GetID(), 2, 1);
		segments[i]->SetGraphics("Line", GetID(), 3, 1);
    segments[i].Double->SetGraphics("Line", GetID(), 4, 1);
    segments[i]->SetGraphics("Line", GetID(), 5, 1);
    if(i>1)
    {
      segments[i].Double->SetGraphics("Line", GetID(), 7, 1);
      segments[i]->SetGraphics("Line", GetID(), 8, 1);
    }
    segments[i]->SetSolidMask(6,0,7,3,-5,9);
		if(i > 1 && i < GetLength(segments)-1)
    {
      segments[i].Plank = 1;
			segments[i]->SetGraphics("Segment", GetID(), 6, 1);
      segments[i]->SetClrModulation(HSL(255,0,128+Random(128)), 6);
    }
    if(i%2 == 0)
    {
      var color = RGB(200,200,200);
      segments[i]->SetClrModulation(color, 2);
      segments[i]->SetClrModulation(color, 3);
      segments[i].Double->SetClrModulation(color, 4);
      segments[i]->SetClrModulation(color, 5);
      segments[i].Double->SetClrModulation(color, 7);
      segments[i]->SetClrModulation(color, 8);
    }
	}
	segments[0]->CreateDouble();
  segments[-1]->CreateDouble();
	segments[1]->SetSolidMask();
  segments[-1]->SetSolidMask();
  segments[1]->SetGraphics(nil, nil,6);
}

public func MakeBridge(obj1, obj2)
{
	MirrorSegments = 1;
	SetProperty("Collectible", 0);
  SetCategory(C4D_StaticBack);
	StartRopeConnect(obj1, obj2);
  SetFixed(1,1);
  ConnectPull();
	AddEffect("IntHang", this, 1, 1, this);
  SetAction("Hanging");
  UpdateSegmentOverlays();
}

func FxIntHangTimer()
{
	TimeStep();
  for(var i = 1; i < ParticleCount-1; i++)
    particles[i][2] = [0,segments[i]->~GetLoadWeight()];
}

func SetFragile()
{
  for(var i = 0; i < ParticleCount; i++)
    segments[i].fragile = 1;
}

/* --------------------- Callbacks form the rope ---------------------- */

/* To be overloaded for special segment behaviour */
private func CreateSegment(int index, object previous)
{
	var segment;
	segment = CreateObject(Ropebridge_Segment);
  segment->SetMaster(this);
	return segment;
}

private func DeleteSegment(object segment, previous)
{
	if(segment)
		segment->RemoveObject();
}

/* When the last segment is removed */
private func RopeRemoved()
{
	RemoveObject();
}

/* --------------------- Graphics of segments ---------------------- */
func DrawRopeLine(start, end, i, int index)
{
  var diff = Vec_Sub(end,start);
  var diffangle = Vec_Angle(diff, [0,0]);
  var point = Vec_Add(start, Vec_Div(diff, 2));
  var length = Vec_Length(diff)*1000/Ladder_Precision/8+100;

  if(index != 4 && index != 7)
  SetLineTransform(segments[i], -diffangle, point[0]*10-particles[i][0][0]*10,point[1]*10-particles[i][0][1]*10+2000, length, index);
  else if(segments[i].Double)
    SetLineTransform(segments[i].Double, -diffangle, point[0]*10-particles[i][0][0]*10,point[1]*10-particles[i][0][1]*10+2000, length, index);
    
}

func DrawRopeLine2(start, end, i, int index)
{
  var diff = Vec_Sub(end,start);
  var diffangle = Vec_Angle(diff, [0,0]);
  var point = Vec_Add(start, Vec_Div(diff, 2));

  SetLineTransform(segments[i], -diffangle, point[0]*10-particles[i][0][0]*10,point[1]*10-particles[i][0][1]*10+2000, 1000, 6);
}

func UpdateLines()
{
	var oldangle = Angle(particles[1][0][0], particles[1][0][1], particles[0][0][0], particles[0][0][1]);
	for(var i=1; i < ParticleCount; i++)
	{
		// Update the Position of the Segment
		segments[i]->SetPosition(particles[i][0][0], particles[i][0][1], 0, Rope_Precision);
    if(segments[i].Double)
    segments[i].Double->SetPosition(particles[i][0][0], particles[i][0][1], 0, Rope_Precision);

		// Calculate the angle to the previous segment
		var angle;
		angle = Angle(particles[i][0][0], particles[i][0][1], particles[i-1][0][0], particles[i-1][0][1]);

		// Every segment has not its graphics, but the graphics of the previous segment (or achor for the first)
		// Otherwise the drawing order would be wrong an we would get lines over segments
		
		// Draw the segment as an overlay for the following segment (only the last segment has two graphics (its and the previous)
/*		if(i > 1 && i < GetLength(segments)-1)
			SetLineTransform(segments[i], -oldangle, particles[i-1][0][0]*10-GetPartX(i)*1000,particles[i-1][0][1]*10-GetPartY(i)*1000, 1000, 6, MirrorSegments );*/

    segments[i]->SetR(90+angle);
		// Draw the left line
		var start = GetRopeConnetPosition(i, 0, 0, angle, oldangle);
		var end   = GetRopeConnetPosition(i, 0, 1, angle, oldangle);
    var end1 = start;
    DrawRopeLine(start, end, i, 2);
    if(segments[i].Plank)
      DrawRopeLine2(start, end, i, 6);
    
		// Draw the right line
		var start = GetRopeConnetPosition(i, 1, 0, angle, oldangle);
		var end   = GetRopeConnetPosition(i, 1, 1, angle, oldangle);
    var end2 = start;
    DrawRopeLine(start, end, i, 3);
    
    // Draw the upper left line
    var start = GetRopeConnetPosition(i, 0, 0, angle, oldangle);
    var end   = GetRopeConnetPosition(i, 0, 1, angle, oldangle);
    var end3 = start[:];
    start[1]-=800;end[1]-=800;
    DrawRopeLine(start, end, i, 4);
    
    // Draw the upder right line
    var start = GetRopeConnetPosition(i, 1, 0, angle, oldangle);
    var end   = GetRopeConnetPosition(i, 1, 1, angle, oldangle);
    var end4 = start[:];
    start[1]-=800;end[1]-=800;
    DrawRopeLine(start, end, i, 5);
    
    if(i>1)
    {
      // Draw the upper left line
      var start = end1;
      var end   = end3;
      start[1]+=000;
      end[1]-=800;
      DrawRopeLine(start, end, i, 7);
      
      // Draw the upder right line
      var start = end2;
      var end   = end4;
      start[1]+=000;
      end[1]-=800;
      DrawRopeLine(start, end, i, 8);
    }

		// Remember the angle
		oldangle = angle;
	}
}

static const Ropebridge_Segment_LeftXOffset = 200;
static const Ropebridge_Segment_RightXOffset = -100;
static const Ropebridge_Segment_LeftYOffset = 00;
static const Ropebridge_Segment_RightYOffset = -100;
static const  Ropebridge_Anchor_RightXOffset = 200;
static const  Ropebridge_Anchor_RightYOffset = 0;
static const  Ropebridge_Anchor_LeftXOffset = -150;
static const  Ropebridge_Anchor_LeftYOffset = -100;

func GetRopeConnetPosition(int index, bool fRight, bool fEnd, int angle, int oldangle)
{
  var SegmentOffset = [[Ropebridge_Segment_LeftXOffset,  Ropebridge_Segment_LeftYOffset],
                       [Ropebridge_Segment_RightXOffset, Ropebridge_Segment_RightYOffset]];
  var AnchorOffset =  [[Ropebridge_Anchor_LeftXOffset,   Ropebridge_Anchor_LeftYOffset], 
                       [Ropebridge_Anchor_RightXOffset,  Ropebridge_Anchor_RightYOffset]];
  var point;
  if( (fEnd == 0 && index == 1) || (fEnd == 1 && index == ParticleCount-1) )
  {
    point = [objects[fEnd][0]->GetX(Ladder_Precision), objects[fEnd][0]->GetY(Ladder_Precision)];
    point[0] += -Cos(objects[fEnd][0]->GetR(), AnchorOffset[fRight][0]*(-1+2*fEnd))+Sin(objects[fEnd][0]->GetR(), AnchorOffset[fRight][1]);
    point[1] += -Sin(objects[fEnd][0]->GetR(), AnchorOffset[fRight][0]*(-1+2*fEnd))-Cos(objects[fEnd][0]->GetR(), AnchorOffset[fRight][1]);
  }
  else
  {
    if(fEnd == 0) index -= 1;
    point = particles[index][0][:];
    point[0] += -Cos(oldangle, SegmentOffset[fRight][0]*MirrorSegments)-Sin(oldangle, SegmentOffset[fRight][1]*MirrorSegments);
    point[1] += -Sin(oldangle, SegmentOffset[fRight][0]*MirrorSegments)+Cos(oldangle, SegmentOffset[fRight][1]*MirrorSegments);
  }
  return point;
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

func Hit()
{
	Sound("WoodHit?");
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
