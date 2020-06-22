#appendto Dialogue

/* Donald dialogue */

func Dlg_Donald_1(object clonk)
{
	if (g_golden_shovel)
	{
		if (g_golden_shovel->Contained() == dlg_target) return CallDialogue(clonk, 2, "Shovel");
		if (ObjectDistance(g_golden_shovel) < 20) return CallDialogue(clonk, 1, "Shovel");
	}
	MessageBox("$Donald1$", clonk, dlg_target); // plz help. tools in acid
	return true;
}

func Dlg_Donald_2(object clonk)
{
	MessageBox("$Donald2$", clonk, clonk); // poor u
	return true;
}

func Dlg_Donald_3(object clonk)
{
	MessageBox("$Donald3$", clonk, dlg_target); // it was unique tools
	return true;
}

func Dlg_Donald_4(object clonk)
{
	MessageBox("$Donald4$", clonk, clonk); // i'll look
	SetDialogueProgress(1);
	return StopDialogue();
}

func Dlg_Donald_Shovel1(object clonk)
{
	g_golden_shovel->Enter(dlg_target);
	MessageBox("$DonaldShovel1$", clonk, clonk); // here is shovel
}

func Dlg_Donald_Shovel2(object clonk)
{
	MessageBox("$DonaldShovel2$", clonk, dlg_target); // thanks 4 shovel
	SetDialogueProgress(1);
	return StopDialogue();
}
