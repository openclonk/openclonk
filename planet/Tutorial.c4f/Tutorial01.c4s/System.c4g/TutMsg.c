/* Nachrichtenbox */

global func TutMsg(string message, string portrait_def)
{
	// Defaults
	if (!portrait_def) portrait_def = "Portrait:_DCO::00ff00::1";
	if (!message) return false;
	// Nachricht als reguläre Message; Spieler nicht stoppen
	CustomMessage(message, 0, GetPlayerByIndex(0, C4PT_User),0 /* 150*/,45, 0xffffff, _DCO, portrait_def, MSG_HCenter);
	return true;
}
