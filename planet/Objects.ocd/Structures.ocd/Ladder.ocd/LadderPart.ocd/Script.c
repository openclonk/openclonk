/**
	Ladder Segment
*/

#include Library_Ladder

local index;

// Called from the ladder object to set a master and the segment index.
public func SetMaster(object new_master, int new_index, ...) 
{
	// First perform setting the master in the library function.
	_inherited(new_master, new_index, ...);
	// Then set index and attach to master object.
	index = new_index;
	AddVertex(0, new_master->GetY() - GetY());
	SetAction("Attach", master);
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
	return false;
}

// Returns the segment (start x, start y, end x, end y, angle) on which the clonk can climb.
// The coordinate value must be specified with a precision of a 1000.
public func GetLadderData()
{
	return [
		GetX(1000),
		GetY(1000) + 4000,
		GetX(1000),
		GetY(1000) - 4000,
		0
	];
}

public func OnLadderGrab(object clonk)
{
	if (master)
		master->OnLadderGrab(clonk, this, index);
	return;
}

public func OnLadderClimb(object clonk)
{
	if (master)
		master->OnLadderClimb(clonk, this, index);
	return;
}

public func OnLadderReleased(object clonk)
{
	if (master)
		master->OnLadderReleased(clonk, this, index);
	return;
}

// Main ladder object is saved.
public func SaveScenarioObject() { return false; }

/*-- Graphics --*/

public func SetPreviousLadder(object ladder)
{
	_inherited(ladder);

	if (!this->GetNextLadder())
		SetGraphics("Top");
	else
		SetGraphics(nil);
}

public func SetNextLadder(object ladder)
{
	_inherited(ladder);

	if (!this->GetPreviousLadder())
		SetGraphics("Bottom");
	else
		SetGraphics(nil);
}

/*-- Properties --*/

local ActMap = {
	Attach = {
		Prototype = Action,
		Name = "Attach",
		FacetBase = 1,
		Procedure = DFA_ATTACH,
	},
};

local Plane = 220;