/*--
	Debug log for the AI
	
--*/

static const AI_Logging = false;
static const AI_LoggingCommandStack = false;

// Debug log for AI purposes.
global func AI_Log(string log_message)
{
	if (!AI_Logging)
		return;
	log_message = Format(log_message,Par(1),Par(2),Par(3),Par(4),Par(5),Par(6),Par(7),Par(8),Par(9));
		Log(Format("[AI]%v: %s", this, log_message));
	return;
}

global func AI_LogCommandStack()
{
	if (!this)
		return;
	if (!AI_LoggingCommandStack)
		return;
	Log("====[AI]Command Stack====");
	Log("====For %v ====", this);
	var command_name, i = 0;
	while (command_name = GetCommand(0, i))
	{
		var msg = command_name;
		if (msg == "Call")
			msg = GetCommand(5, i);
		msg = Format("[AI]%s: target: %v target2: %v", msg, GetCommand(1, i), GetCommand(4, i));
		Log(msg);		
		i++;
	}
	Log("====End of Command Stack ====");
	return;
}