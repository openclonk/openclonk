/**
	Simple, static ladder

	Cannot be built ingame and must be placed by designers. Use the SetLength() function to make it as long as you wish.

	@authors: Clonkonaut
*/

local segments;

func Initialize()
{
	// Create ladder segments to climb on.
	CreateSegments();
}

public func SetLength(int segment_count)
{
	if (GetLength(segments) == segment_count) return;
	if (segment_count < 2) return;

	for (var segment in segments)
		segment->RemoveObject();

	var new_height = segment_count * 8;
	SetShape(-3, new_height / -2, 5, new_height);
	CreateSegments();
}

/*-- Ladder Control --*/

// Creates the segments which control the climbing.
func CreateSegments()
{
	segments = [];
	var nr_segments = (GetBottom() - GetTop()) / 8;
	for (var index = 0; index < nr_segments; index++)
	{
		var y = GetTop() + index * 8;
		var segment = CreateObject(MetalLadderSegment, 0, y+4);
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

/*-- Properties --*/

local Name = "$Name$";