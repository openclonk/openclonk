/*-- Buy at flagpole --*/

public func Construction(...)
{
	// This rule enables wealth display because it's annoying to not
	// know your wealth in multiplayer when many people buy/sell at
	// the flagpole)
	// Doesn't remove the display on destruction because there's no
	// refcounting for this command.
	GUI_Controller->ShowWealth();
	return _inherited(...);
}

public func Activate(int by_plr)
{
	MessageWindow(this.Description, by_plr);
	return true;
}

local Name = "$Name$";
local Description = "$Description$";
local Visibility = VIS_Editor;
local EditorPlacementLimit = 1; // Rules are to be placed only once
