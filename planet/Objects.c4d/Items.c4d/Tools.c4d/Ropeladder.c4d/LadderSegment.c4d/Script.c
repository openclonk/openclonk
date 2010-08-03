/*-- Ropeladder_Segment --*/

#include Library_Ladder

local master, index;

public func SetMaster(new_master, new_index) { master = new_master; index = new_index; }

public func GetLadderData()
{
	if(master != nil)
	{
		return master->~GetLadderData(index);
	}
	return _inherited();
}

public func LogLadderData()
{
	var startx, starty, endx, endy;
	if(master != nil)
	{
		master->~GetLadderData(index, startx, starty, endx, endy);
		return;
	}
}

public func OnLadderGrab(clonk)
{
	if(master != nil)
		master->OnLadderGrab(clonk, index);
}

public func OnLadderClimb(clonk)
{
	if(master != nil)
		master->OnLadderClimb(clonk, index);
}