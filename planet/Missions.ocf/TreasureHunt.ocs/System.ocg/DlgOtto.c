#appendto Dialogue

/* Otto dialogue */

func Dlg_Otto_1(object clonk)
{
	MessageBox("$Otto1$", clonk, dlg_target); // luv it here
	return true;
}

func Dlg_Otto_2(object clonk)
{
	MessageBox("$Otto2$", clonk, dlg_target); // look @ that island
	SetDialogueProgress(1);
	return StopDialogue();
}
