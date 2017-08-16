#appendto Dialogue

/* Jane dialogue */

func Dlg_Jane_1(object clonk)
{
	var msg = "$Jane1$"; // he's swinging all day instead of looking 4 gold
	if (!npc_tarzan || !npc_tarzan->GetAlive()) msg = "$Jane1B$"; // he's dead instead of looking 4 gold
	MessageBox(msg, clonk, dlg_target);
	return true;
}

func Dlg_Jane_2(object clonk)
{
	var msg = "$Jane2$";
	if (!npc_tarzan || !npc_tarzan->GetAlive()) msg = "$Jane2B$";
	MessageBox(msg, clonk, clonk); // i want rope 2
	return true;
}

func Dlg_Jane_3(object clonk)
{
	MessageBox("$Jane3$", clonk, dlg_target); // look chest
	return true;
}

func Dlg_Jane_4(object clonk)
{
	MessageBox("$Jane4$", clonk, dlg_target); // u can carry 2
	return true;
}

func Dlg_Jane_5(object clonk)
{
	MessageBox("$Jane5$", clonk, dlg_target); // take all plz
	SetDialogueProgress(1);
	return StopDialogue();
}
