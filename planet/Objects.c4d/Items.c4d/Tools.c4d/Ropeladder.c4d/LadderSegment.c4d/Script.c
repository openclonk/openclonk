/*-- Ropeladder_Segment --*/

#include Library_Ladder

local master, index;
local angle;

public func SetAngle(int new_angle) {	angle = new_angle; }

public func SetMaster(new_master, new_index) { master = new_master; index = new_index; }

public func CanNotBeClimbed(bool fClimbing)
{
	var test_height = 10;
	if(fClimbing) test_height = 8;

	if(GBackSolid(1, test_height) && GBackSolid(-1, test_height)) return true;
	if( (angle>45 && angle < 360-45) && GBackSolid(0, test_height)) return true;
	
	return false;
}

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