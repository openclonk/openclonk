/**
	AI Debugging
	Functionality that helps to debug AI control.
	
	@author Maikel
*/

// Callback from the Definition()-call
public func OnDefineAI(proplist def)
{
	_inherited(def);

	def->GetControlEffect().DebugLoggingOn = false; // Whether or not debug logging is turned on.
}

public func LogAI(effect fx, string message)
{
	if (fx.DebugLoggingOn)
		Log("[%d]AI WARNING (%v): %s", FrameCounter(), fx.Target, message);
	return;
}
