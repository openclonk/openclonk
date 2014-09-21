#appendto Dialogue

/* Ann dialogue */

func Dlg_Ann_1(object clonk)
{
	MessageBox("$Ann1$", clonk, dlg_target); // me stuck
	return true;
}

func Dlg_Ann_2(object clonk)
{
	MessageBox("$Ann2$", clonk, dlg_target); // switch out of range
	return true;
}

func Dlg_Ann_3(object clonk)
{
	MessageBox("$Ann3$", clonk, clonk); // no way around?
	return true;
}

func Dlg_Ann_4(object clonk)
{
	MessageBox("$Ann4$", clonk, dlg_target); // nope. need pipe.
	return true;
}

func Dlg_Ann_5(object clonk)
{
	MessageBox("$Ann5$", clonk, clonk); // hm i should find pipe
	SetDialogueProgress(1);
	return StopDialogue();
}
