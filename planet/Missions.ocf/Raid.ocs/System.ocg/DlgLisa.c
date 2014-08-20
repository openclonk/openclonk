#appendto Dialogue

/* Lisa Dialogue */

func Dlg_Lisa_1(object clonk)
{
	MessageBox("$Lisa1$", clonk, dlg_target); // we're making progress
	return true;
}

func Dlg_Lisa_2(object clonk)
{
	MessageBox("$Lisa2$", clonk, clonk); // why are you building?
	return true;
}

func Dlg_Lisa_3(object clonk)
{
	MessageBox("$Lisa3$", clonk, dlg_target); // we're being attacked
	return true;
}

func Dlg_Lisa_4(object clonk)
{
	MessageBox("$Lisa4$", clonk, clonk); // by whom?
	return true;
}

func Dlg_Lisa_5(object clonk)
{
	MessageBox("$Lisa5$", clonk, dlg_target); // later. help with the site now.
	StopDialogue();
	SetDialogueProgress(6);
	return true;
}

func Dlg_Lisa_6(object clonk)
{
	MessageBox("$Lisa6$", clonk, dlg_target); // help with the site now.
	StopDialogue();
	SetDialogueProgress(6);
	return true;
}

// post attack dialogue
func Dlg_Lisa_100(object clonk)
{
	MessageBox("$Lisa100$", clonk, dlg_target); // plz do sth
	StopDialogue();
	SetDialogueProgress(100);
	return true;
}

func Dlg_Lisa_200(object clonk)
{
	MessageBox("$Lisa200$", clonk, dlg_target); // thx for helping
	StopDialogue();
	SetDialogueProgress(200);
	return true;
}

