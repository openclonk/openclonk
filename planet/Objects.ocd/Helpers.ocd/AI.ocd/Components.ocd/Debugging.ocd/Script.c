/**
	AI Debugging
	Functionality that helps to debug AI control.
	
	@author Maikel
*/

// Callback from the Definition()-call.
public func OnDefineAI(proplist def)
{
	_inherited(def);
	// Whether or not debug logging is turned on.
	def->GetControlEffect().DebugLoggingOn = false;
}

// Logs AI warnings.
public func LogAI_Warning(effect fx, string message)
{
	if (fx.DebugLoggingOn)
		Log("[%d] AI WARNING (%v): %s", FrameCounter(), fx.Target, message);
	return;
}

// Logs AI info.
public func LogAI_Info(proplist fx, string message)
{
	if (fx.DebugLoggingOn)
		Log("[%d] AI INFO    (%v): %s", FrameCounter(), fx.Target, message);
	return;
}

// Logs the call stack.
public func LogAI_CallStack(proplist fx)
{
	if (fx.DebugLoggingOn)
	{
		Log("[%d] AI CALLSTACK", FrameCounter());
		LogCallStack();
	}
	return;
}