/* Nachrichten fuers Intro */

func MessageBoxAll(string message, object talker, bool permanent)
{
	if (permanent) permanent = "@"; else permanent = "";
	message = Format("%s<c %x>%s:</c> %s", permanent, talker->GetColor(), talker->GetName(), message);
	CustomMessage(message, nil, NO_OWNER, 150,150, nil, GUI_MenuDeco, Dialogue);
}
