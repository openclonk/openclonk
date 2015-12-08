/**
	Ladder Control
	Containes the basic functionality for ladders.

	@author Randrian
*/

local next_segment;
local prev_segment;

public func IsLadder() { return true; }

// Returns the segment (start x, start y, end x, end y, angle) on which the clonk can climb.
public func GetLadderData()
{
	// Normally (if not overloaded) interpret the first vertex as start and the second as end.
	return [
		GetX() + GetVertex(0, 0),
		GetY() + GetVertex(0, 1),
		GetX() + GetVertex(1, 0),
		GetY() + GetVertex(1, 1),
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
