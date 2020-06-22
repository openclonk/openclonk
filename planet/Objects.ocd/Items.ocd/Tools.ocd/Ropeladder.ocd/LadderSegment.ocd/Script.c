/**
	Ropeladder Segment

	@author Randrian	
*/

#include Library_Ladder

local index;
local angle;

public func SetAngle(int new_angle) 
{ 
	angle = new_angle;
}

// Called from the ladder object to set a master and the segment index.
public func SetMaster(object new_master, int new_index, ...) 
{
	// First perform setting the master in the library function.
	_inherited(new_master, new_index, ...);
	index = new_index;
	return;
}

// Returns whether the ladder can be climbed.
public func CanNotBeClimbed(bool is_climbing)
{
	var test_height = 10;
	if (is_climbing) 
		test_height = 8;

	if (GBackSolid(1, test_height) && GBackSolid(-1, test_height)) 
		return true;
	if (Inside(angle, 45, 315) && GBackSolid(0, test_height))
		return true;
	return false;
}

public func GetLadderData()
{
	if (master)
		return master->~GetLadderData(index);
	return _inherited(...);
}

public func OnLadderGrab(object clonk)
{
	if (master)
		master->OnLadderGrab(clonk, index);
}

public func OnLadderClimb(clonk)
{
	if (master)
		master->OnLadderClimb(clonk, index);
}

// Main ladder object is saved
func SaveScenarioObject() { return false; }
