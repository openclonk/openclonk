/**
	Ladder Control
	Containes the basic functionality for ladder segments.

	@author Randrian
*/

local master;
local next_segment;
local prev_segment;

// Ladders are being searched for by FindObject and friends using a property.
local IsLadder = true;

public func SetMaster(object new_master, int new_index)
{
	master = new_master;
	return;
}

public func GetMaster()
{
	return master;
}

// Returns whether this segment and the other one are from the same ladder.
public func IsSameLadder(object other_segment)
{
	return GetMaster() == other_segment->GetMaster();
}

// Returns the segment (start x, start y, end x, end y, angle) on which the clonk can climb.
// The coordinate value must be specified with a precision of a 1000.
public func GetLadderData()
{
	// Normally (if not overloaded) interpret the first vertex as start and the second as end.
	return [
		GetX(1000) + 1000 * GetVertex(0, 0),
		GetY(1000) + 1000 * GetVertex(0, 1),
		GetX(1000) + 1000 * GetVertex(1, 0),
		GetY(1000) + 1000 * GetVertex(1, 1),
		0
	];
}

// Get the connected previous segment.
public func GetPreviousLadder()
{
	return prev_segment;
}

// Get the connected next segment.
public func GetNextLadder()
{
	return next_segment;
}

// Set the connected previous segment.
public func SetPreviousLadder(object ladder)
{
	prev_segment = ladder;
}

// Set the connected next segment.
public func SetNextLadder(object ladder)
{
	next_segment = ladder;
}
