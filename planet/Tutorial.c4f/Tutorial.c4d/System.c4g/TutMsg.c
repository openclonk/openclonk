/*-- Message Box --*/

global func TutMsg(string message, string portrait_def)
{
	// Defaults
	if (!portrait_def) 
		portrait_def = "Portrait:_DCO::00ff00::1";
	if (!message)
		return false;
	// Message as regular one, don't stop the player.
	CustomMessage(message, 0, GetPlayerByIndex(0, C4PT_User), 0 /* 150*/, 45, 0xffffff, _DCO, portrait_def, MSG_HCenter);
	return true;
}
