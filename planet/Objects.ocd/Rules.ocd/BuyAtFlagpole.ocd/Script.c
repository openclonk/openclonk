/*-- Buy at flagpole --*/

protected func Activate(int iByPlayer)
{
	MessageWindow(GetProperty("Description"), iByPlayer);
	return true;
}

local Name = "$Name$";
local Description = "$Description$";
