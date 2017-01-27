/**
	AI Debugging
	Functionality that helps to debug AI control.
	
	@author Maikel
*/


// AI Settings.
local DebugLoggingOn = false; // Whether or not debug logging is turned on.


public func LogAI(effect fx, string message)
{
	if (fx.control.DebugLoggingOn)
		Log("[%d]AI WARNING (%v): %s", FrameCounter(), fx.Target, message);
	return;
}