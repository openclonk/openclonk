/**
	Simple, static ladder

	Cannot be built ingame and must be placed by designers. Use the SetLength() function to make it as long as you wish.

	@authors: Clonkonaut
*/

local segments;
local SegmentHeight = 8;

func Initialize()
{
	// Create ladder segments to climb on.
	CreateSegments();
}

private func GetSegmentCount()
{
	return GetLength(segments);
}

public func SetLength(int segment_count, int extension_direction)
{
	// Adjust the length of the ladder
	// segment_count: New number of segments
	// extension_direction: Which end of the ladder to expand/shrink. 0: Top and bottom. +1: Bottom. -1: Top
	var old_segment_count = GetSegmentCount();
	if (old_segment_count == segment_count) return;
	if (segment_count < 2) return;

	for (var segment in segments)
		segment->RemoveObject();
		
	var new_height = segment_count * SegmentHeight;
	SetShape(-3, new_height / -2, 5, new_height);
	
	if (extension_direction)
	{
		var height_diff = (segment_count - old_segment_count) * SegmentHeight;
		MovePosition(0, height_diff/2 * extension_direction);
	}

	CreateSegments();
}

/*-- Ladder Control --*/

// Creates the segments which control the climbing.
func CreateSegments()
{
	segments = [];
	var nr_segments = (GetBottom() - GetTop()) / SegmentHeight;
	for (var index = 0; index < nr_segments; index++)
	{
		var y = GetTop() + index * SegmentHeight;
		var segment = CreateObject(MetalLadderSegment, 0, y + 4);
		segment->SetMaster(this, index);
		// Store the segments.
		PushBack(segments, segment);
		// Set next and previous segment for climbing control.
		if (index > 0)
		{
			segments[index - 1]->SetPreviousLadder(segment);
			segment->SetNextLadder(segments[index - 1]);
		}
	}
}

public func OnLadderGrab(object clonk, object segment, int segment_index)
{
	segment->Sound("Clonk::Action::Grab");
}

public func OnLadderClimb(object clonk, object segment, int segment_index)
{
	if (clonk->GetComDir() == COMD_Up || clonk->GetComDir() == COMD_Down)
		if (!Random(20))
			segment->Sound("Clonk::Movement::Rustle?", {volume = 35});
}

public func OnLadderReleased(object clonk, object segment, int segment_index)
{
	segment->Sound("Clonk::Action::UnGrab", {volume = 50});
}

/*-- Saving --*/

// Save placed ladder segments in scenarios.
public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) 
		return false;

	props->AddCall("Length", this, "SetLength", GetLength(segments));
	return true;
}


/* Editor dragging */

private func GetTopHandle() { return [0, GetSegmentCount() * SegmentHeight / -2]; }
private func SetTopHandle(pt) { SetLength(Max(2, (GetBottom() - pt[1] + SegmentHeight/2) / SegmentHeight), -1); }
private func GetBottomHandle() { return [0, GetSegmentCount() * SegmentHeight / 2]; }
private func SetBottomHandle(pt) { SetLength(Max(2, (pt[1] - GetTop() + SegmentHeight/2) / SegmentHeight), +1); }

public func Definition(def)
{
	// Drag handles for ladder length
	if (!def.EditorProps) def.EditorProps = {};
	def.EditorProps.top_handle = {
	  Name="$TopHandle$",
	  Type="point",
	  Relative = true,
	  HorizontalFix = true,
	  AsyncGet="GetTopHandle",
	  Set="SetTopHandle",
	  Priority = 1,
	  Color = 0x80ff00 };
	def.EditorProps.bottom_handle = {
	  Name="$BottomHandle$",
	  Type="point",
	  Relative = true,
	  HorizontalFix = true,
	  AsyncGet="GetBottomHandle",
	  Set="SetBottomHandle",
	  Color = 0xff8000 };
}


/*-- Properties --*/

local Name = "$Name$";
local Plane = 221;
