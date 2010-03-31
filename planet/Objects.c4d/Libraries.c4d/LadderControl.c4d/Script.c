/*--
	Ladder control
	Authors: Randrian

	Containes the basic functionality for ladders.
--*/

local next_segment;
local prev_segment;

public func IsLadder() { return true; }

// Returns the segment (start position, end position) on which the clonk can climb.
public func GetLadderData(&startx, &starty, &endx, &endy, &angle)
{
	// Normally (if not overloaded) interpret the first vertex as start and the second as end
	startx = GetX()+GetVertex(0, 0);
	starty = GetY()+GetVertex(0, 1);
	endx   = GetX()+GetVertex(1, 0);
	endy   = GetY()+GetVertex(1, 1);
	angle  = 0;
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