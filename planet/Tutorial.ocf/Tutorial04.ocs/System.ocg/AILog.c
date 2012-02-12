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
		Log(Format("[AI]%s: %s", this->GetName(), log_message));
	return;
}

global func AI_LogCommandStack()
{
	if (!this)
		return;
	if (!AI_LoggingCommandStack)
		return;
	Log("====[AI]Command Stack====");
	Log("====For %s====", this->GetName());
	var command_name, i = 0;
	while (command_name = GetCommand(0, i))
	{
		var msg = command_name;
		if (msg == "Call")
			msg = GetCommand(5, i);
		var t1 = GetCommand(1, i);
		if (t1)
			t1 = t1->GetName();
		else
			t1 = Format("%d", GetCommand(2, i));
		var t2 = GetCommand(4, i);
		if (t2)
			t2 = t2->GetName();
		else
			t2 = Format("%d", GetCommand(3, i));
		msg = Format("[AI]%s: target: %s target2: %s", msg, t1, t2);
		Log(msg);		
		i++;
	}
	Log("====End of Command Stack ====");
	return;
}