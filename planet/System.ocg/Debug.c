/**
	Debug.c
	Functions for debugging.

	@author Zapper
*/


/**
 Logs a warning with the call stack.

 @par message The warning message, no need to include "WARNING" or similar, this is already included here.
              Accepts additional parameters as a normal log message would.
 @par ... additional formatting parameters for the message.
 */
global func Warning(string message, ...)
{
	DebugLog("WARNING: %s", Format(message, ...));
	LogCallStack();
}
