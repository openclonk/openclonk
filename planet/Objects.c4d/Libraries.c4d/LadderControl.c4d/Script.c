/*--
	Ladder control
	Authors: Randrian

	Containes the basic functionality for ladders.
--*/

local next_segment;
local prev_segment;

public func IsLadder() { return true; }

// Returns the segment (start position, end position) on which the clonk can climb.
public func GetLadderData()
{
	// Normally (if not overloaded) interpret the first vertex as start and the second as end
	return [
		GetX() + GetVertex(0, 0),
		GetY() + GetVertex(0, 1),
		GetX() + GetVertex(1, 0),
		GetY() + GetVertex(1, 1),
		0];
}

// Get the connected segments
public func GetPreviousLadder()
{
	return prev_segment;
}

public func GetNextLadder()
{
	return next_segment;
}

// Set the connected segments
public func SetPreviousLadder(object ladder)
{
	prev_segment = ladder;
}

public func SetNextLadder(object ladder)
{
	next_segment = ladder;
}