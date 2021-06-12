/*--
		Override screenshot functionality
--*/

global func PlayerControl(proplist plr, int ctrl, ...)
{
	if (ctrl == CON_TryScreenshot)
	{
		CustomMessage(Format("$MsgCheater$", plr->GetTaggedName()));
		Sound("UI::Error", true);
		//var crew = plr->GetCursor(); - used for cheating
		//if (crew) crew->Punch(crew, 50);
		return true;
	}
	return _inherited(plr, ctrl, ...);
}