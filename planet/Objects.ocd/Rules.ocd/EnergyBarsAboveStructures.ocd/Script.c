/*-- energy bars above buildings --*/

/*
	The energy bar itself is created in the structure library.
*/

protected func Activate(int iByPlayer)
{
	MessageWindow(GetProperty("Description"), iByPlayer);
	return true;
}

local Name = "$Name$";
local Description = "$Description$";
