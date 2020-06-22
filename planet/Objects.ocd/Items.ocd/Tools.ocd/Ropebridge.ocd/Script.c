/**
	Rope bridge
	A bridge consisting of single wooden planks tied together with ropes.

	@author Randrian
*/

#include Library_Rope

local RopeBridge_Precision = 100;

local bridge_post1;
local bridge_post2;


/*-- Bridge Creation --*/

// Create a rope bridge starting at (x1, y1) and ending at (x2, y2).
// Set fragile to make the steps break when pressured.
// Set portable to make a bridge which can be retracted.
public func Create(int x1, int y1, int x2, int y2, bool fragile, bool portable)
{
	if (this != Ropebridge)
		return;
	var post1 = CreateObjectAbove(Ropebridge_Post, x1, y1);
	var post2 = CreateObjectAbove(Ropebridge_Post, x2, y2);
	var bridge = CreateObject(Ropebridge, (x1 + x2) / 2, (y1 + y2) / 2);
	bridge->MakeBridge(post1, post2);
	post1->SetStatic(!portable);
	post2->SetStatic(!portable);
	if (fragile)
		bridge->SetFragile();
	return;
}

// Creates the rope bridge between the two bridge posts.
public func MakeBridge(object post1, object post2)
{
	if (post1->GetID() != Ropebridge_Post || post2->GetID() != Ropebridge_Post)
		return FatalError("Ropebridge can only be connected between bridge posts");
	// Change the properties of the ropebridge object.
	this.Collectible = false;
	SetCategory(C4D_StaticBack);
	// Swap the posts if they have inverted x-coordinates.
	if (post1->GetX() > post2->GetX())
	{
		var swap = post1;
		post1 = post2;
		post2 = swap;	
	}
	// Set the bridge posts.
	bridge_post1 = post1;
	bridge_post2 = post2;
	bridge_post1->SetBridge(this);
	bridge_post2->SetBridge(this);
	bridge_post1->Turn(DIR_Left);
	bridge_post2->Turn(DIR_Right);
	StartRopeConnect(bridge_post1, bridge_post2);
	SetFixed(true, true);
	ConnectPull();
	AddEffect("IntHang", this, 1, 1, this);
	SetAction("Hanging");
	UpdateSegmentOverlays();
}

public func SetFragile()
{
	for (var i = 0; i < lib_rope_particle_count; i++)
		lib_rope_segments[i]->SetFragile(true);
}


/*-- Usage --*/

public func RejectUse(object clonk)
{
	return !clonk->IsWalking();
}

public func ControlUse(object clonk, int x, int y)
{
	// Create the first bridge post at the clonks feet.
	if (!bridge_post1)
	{
		bridge_post1 = CreateObjectAbove(Ropebridge_Post, 0, clonk->GetBottom());
		return true;
	}
	// Check whether the location of the second post is fine.
	var from_x = bridge_post1->GetX();
	var from_y = bridge_post1->GetY() + clonk->GetBottom();
	var to_x = clonk->GetX();
	var to_y = clonk->GetY() + clonk->GetBottom();
	var distance = Distance(from_x, from_y, to_x, to_y);
	var angle = Angle(from_x, from_y, to_x, to_y);
	if (distance > 120)
	{
		clonk->Message("$MsgBridgeLong$");	
		return true;
	}
	if (distance < 40)
	{
		clonk->Message("$MsgBridgeShort$");	
		return true;
	}
	if (!Inside(angle, 240, 300) && !Inside(angle, 60, 120))
	{
		clonk->Message("$MsgBridgeUneven$");	
		return true;
	}
	// Create the second bridge post at the clonks feet.
	bridge_post2 = CreateObjectAbove(Ropebridge_Post, 0, clonk->GetBottom());
	// Exit the ropebridge and connect between the two posts.
	Exit();
	MakeBridge(bridge_post1, bridge_post2);
	return true;
}


/*-- Rope Mechanics --*/

public func FxIntHangTimer()
{
	// Do a time step of the rope library.
	TimeStep();
	// Then apply forces from objects standing on the bridge.
 	for (var i = 1; i < lib_rope_particle_count - 1; i++)
 	{
   		lib_rope_particles[i].accx = 0;
   		lib_rope_particles[i].accy = lib_rope_segments[i]->~GetLoadWeight();
   	}
   	return FX_OK;
}

// To be overloaded for special segment behaviour.
private func CreateSegment(int index, object previous)
{
	var segment = CreateObjectAbove(Ropebridge_Segment);
	segment->SetMaster(this);
	return segment;
}

private func DeleteSegment(object segment, object previous)
{
	if (segment)
		segment->RemoveObject();
}

// Called if either of the posts got destroyed: consider the bridge lost and remove it.
public func OnBridgePostDestruction()
{
	RemoveObject();
	return;
}

public func Destruction()
{
	// Make sure both bridge posts are removed as well.
	if (bridge_post1)
	{
		bridge_post1->SetBridge(nil);
		bridge_post1->RemoveObject();
	}
	if (bridge_post2)
	{
		bridge_post2->SetBridge(nil);
		bridge_post2->RemoveObject();
	}
	return _inherited(...);
}


/*-- Segment Graphics --*/

public func DrawRopeLine(array start, array end, int i, int index)
{
	var angle = -Angle(end[0], end[1], start[0], start[1]);
	var point_x = 5 * (start[0] + end[0]);
	var point_y = 5 * (start[1] + end[1]);
	var length = Vec_Length(Vec_Sub(end, start)) * 125 / RopeBridge_Precision + 100;

	if (index != 4 && index != 7)
		SetLineTransform(lib_rope_segments[i], angle, point_x - lib_rope_particles[i].x * 10, point_y - lib_rope_particles[i].y * 10 + 2000, length, index);
	else if (lib_rope_segments[i]->GetDouble())
    	SetLineTransform(lib_rope_segments[i]->GetDouble(), angle, point_x - lib_rope_particles[i].x * 10, point_y - lib_rope_particles[i].y * 10 + 2000, length, index);
    return;
}

public func DrawPlank(array start, array end, int i)
{
	var angle = -Angle(end[0], end[1], start[0], start[1]);
	var point_x = 5 * (start[0] + end[0]);
	var point_y = 5 * (start[1] + end[1]);
	SetLineTransform(lib_rope_segments[i], angle, point_x - lib_rope_particles[i].x * 10, point_y - lib_rope_particles[i].y * 10 + 2000, 1000, 6);
	return;
}

public func UpdateLines()
{
	var oldangle = Angle(lib_rope_particles[1].x, lib_rope_particles[1].y, lib_rope_particles[0].x, lib_rope_particles[0].y);
	for (var i = 1; i < lib_rope_particle_count; i++)
	{
		var segment = lib_rope_segments[i];
		var particle = lib_rope_particles[i];
		// Update the position of the segment.
		if (segment)
		{
			segment->SetPosition(particle.x, particle.y, 0, LIB_ROPE_Precision);
			if (segment->GetDouble())
				segment->GetDouble()->SetPosition(particle.x, particle.y, 0, LIB_ROPE_Precision);
		}
		
		// Calculate the angle to the previous segment.
		var prev_particle = lib_rope_particles[i - 1];
		var angle = Angle(particle.x, particle.y, prev_particle.x, prev_particle.y);

		// Every segment has not its graphics, but the graphics of the previous segment (or achor for the first).
		// Otherwise the drawing order would be wrong an we would get lines over segments.
		
		// Draw the segment as an overlay for the following segment (only the last segment has two graphics: its and the previous).
		segment->SetR(90 + angle);
		// Draw the left line.
		var start = GetRopeConnectPosition(i, 0, 0, angle, oldangle);
		var end = GetRopeConnectPosition(i, 0, 1, angle, oldangle);
    	var end1 = start;
    	DrawRopeLine(start, end, i, 2);
    	if (segment->HasPlank())
      		DrawPlank(start, end, i);
    
		// Draw the right line.
		var start = GetRopeConnectPosition(i, 1, 0, angle, oldangle);
		var end = GetRopeConnectPosition(i, 1, 1, angle, oldangle);
		var end2 = start;
    	DrawRopeLine(start, end, i, 3);
    	
		// Draw the upper left line.
		var start = GetRopeConnectPosition(i, 0, 0, angle, oldangle);
		var end = GetRopeConnectPosition(i, 0, 1, angle, oldangle);
		var end3 = start[:];
		start[1] -= 800;
		end[1] -= 800;
		DrawRopeLine(start, end, i, 4);
	
		// Draw the upper right line.
		var start = GetRopeConnectPosition(i, 1, 0, angle, oldangle);
		var end = GetRopeConnectPosition(i, 1, 1, angle, oldangle);
		var end4 = start[:];
		start[1] -= 800;
		end[1] -= 800;
		DrawRopeLine(start, end, i, 5);
	    
	    if (i > 1)
	    {
			// Draw the upper left line.
			var start = end1;
			var end = end3;
			end[1] -= 800;
			DrawRopeLine(start, end, i, 7);
	      
			// Draw the upper right line.
			var start = end2;
			var end = end4;
			end[1] -= 800;
			DrawRopeLine(start, end, i, 8);
	    }
		// Remember the angle.
		oldangle = angle;
	}
	return;
}

public func UpdateSegmentOverlays()
{
	for (var i = 1; i < GetLength(lib_rope_segments); i++)
	{
    	var segment = lib_rope_segments[i];
    	segment->CreateDouble();
		segment->SetGraphics("Line", GetID(), 2, GFXOV_MODE_Base);
		segment->SetGraphics("Line", GetID(), 3, GFXOV_MODE_Base);
    	segment->GetDouble()->SetGraphics("Line", GetID(), 4, GFXOV_MODE_Base);
    	segment->SetGraphics("Line", GetID(), 5, GFXOV_MODE_Base);
	    if (i > 1)
	    {
			segment->GetDouble()->SetGraphics("Line", GetID(), 7, GFXOV_MODE_Base);
			segment->SetGraphics("Line", GetID(), 8, GFXOV_MODE_Base);
	    }
		segment->SetSolidMask(6, 0, 7, 3, -5, 9);
		if (i > 1 && i < GetLength(lib_rope_segments) - 1)
   		{
			segment->SetPlank(true);
			segment->SetGraphics("Segment", GetID(), 6, GFXOV_MODE_Base);
      		segment->SetClrModulation(HSL(255, 0, 128 + Random(128)), 6);
    	}
		if (i % 2 == 0)
		{
			var color = RGB(200, 200, 200);
			segment->SetClrModulation(color, 2);
			segment->SetClrModulation(color, 3);
			segment->GetDouble()->SetClrModulation(color, 4);
			segment->SetClrModulation(color, 5);
			segment->GetDouble()->SetClrModulation(color, 7);
			segment->SetClrModulation(color, 8);
    	}
	}
	lib_rope_segments[0]->CreateDouble();
	lib_rope_segments[-1]->CreateDouble();
	lib_rope_segments[1]->SetSolidMask();
	lib_rope_segments[-1]->SetSolidMask();
	lib_rope_segments[1]->SetGraphics(nil, nil, 6);
	return;
}

static const Ropebridge_Segment_Offset = [[200, 0], [-100, -100]];
static const Ropebridge_Anchor_Offset = [[-150, -100], [200, 0]];

public func GetRopeConnectPosition(int index, int right, int end, int angle, int oldangle)
{
	if ((!end && index == 1) || (end && index == lib_rope_particle_count - 1))
	{
		var obj = lib_rope_objects[end][0];
		var point_x = obj->GetX(RopeBridge_Precision) - Cos(obj->GetR(), Ropebridge_Anchor_Offset[right][0] * (-1 + 2 * end)) + Sin(obj->GetR(), Ropebridge_Anchor_Offset[right][1]);
		var point_y = obj->GetY(RopeBridge_Precision) - Sin(obj->GetR(), Ropebridge_Anchor_Offset[right][0] * (-1 + 2 * end)) - Cos(obj->GetR(), Ropebridge_Anchor_Offset[right][1]);
		return [point_x, point_y];
	}
	if (!end) 
		index -= 1;
	var point_x = lib_rope_particles[index].x - Cos(oldangle, Ropebridge_Segment_Offset[right][0]) - Sin(oldangle, Ropebridge_Segment_Offset[right][1]); 
	var point_y = lib_rope_particles[index].y - Sin(oldangle, Ropebridge_Segment_Offset[right][0]) + Cos(oldangle, Ropebridge_Segment_Offset[right][1]);
	return [point_x, point_y];
}

public func SetLineTransform(object obj, int r, int xoff, int yoff, int length, int layer, int mirror)
{
	if (!mirror) 
		mirror = 1;
	var fsin = Sin(r, 1000), fcos = Cos(r, 1000);
	// Set matrix values.
	obj->SetObjDrawTransform (
		+fcos * mirror, +fsin * length / 1000, xoff,
		-fsin * mirror, +fcos * length / 1000, yoff, layer
	);
}

public func Hit()
{
	Sound("Hits::Materials::Wood::WoodHit?");
}

public func IsInventorProduct() { return true; }


/*-- Scenario Saving --*/

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	props->Remove("Category");
	// Save bridge creation on scenario save.
	if (lib_rope_objects && lib_rope_objects[0] && lib_rope_objects[1])
	{
		var o1 = lib_rope_objects[0][0], o2 = lib_rope_objects[1][0];
		if (o1 && o2) props->AddCall("Bridge", this, "MakeBridge", o1, o2);
	}
	return true;
}


/*-- Properties --*/

local ActMap = {
	Hanging = {
		Prototype = Action,
		Name = "Hanging",
		Width = 0,
		Height = 0
	},
};
local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;
local Components = {Wood = 2/*, Rope = 2*/};
