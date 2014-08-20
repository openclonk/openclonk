#appendto Dialogue

/* Matthi Dialogue */

func Dlg_Matthi_1(object clonk)
{
	// If the player already has a sproutberry, he won't get another offer
	if (clonk->FindContents(Sproutberry))
		if (g_pyrit_spoken)
			return Dlg_Matthi_100(clonk); // asking for oil
		else
			return Dlg_Matthi_6(clonk); // just a generic remark
	MessageBox("$Matthi1$", clonk, dlg_target); // u want berry?
	return true;
}

func Dlg_Matthi_2(object clonk)
{
	MessageBox("$Matthi2$", clonk, clonk); // can't pay
	return true;
}

func Dlg_Matthi_3(object clonk)
{
	MessageBox("$Matthi3$", clonk, dlg_target); // no prob it's free
	clonk->CreateContents(Sproutberry);
	return true;
}

func Dlg_Matthi_4(object clonk)
{
	MessageBox("$Matthi4$", clonk, clonk); // free beer or free speech?
	return true;
}

func Dlg_Matthi_5(object clonk)
{
	MessageBox("$Matthi5$", clonk, dlg_target); // what?
	return true;
}

func Dlg_Matthi_6(object clonk)
{
	MessageBox("$Matthi6$", clonk, dlg_target); // sproutberries r delicous!
	StopDialogue();
	SetDialogueProgress(1);
	return true;
}


/* After speaking to Pyrit: Ask for oil */

func Dlg_Matthi_100(object clonk)
{
	MessageBox("$Matthi100$", clonk, clonk); // where is oil?
	SetDialogueProgress(101);
	return true;
}

func Dlg_Matthi_101(object clonk)
{
	MessageBox("$Matthi101$", clonk, dlg_target); // i h8 oil
	return true;
}

func Dlg_Matthi_102(object clonk)
{
	MessageBox("$Matthi102$", clonk, dlg_target); // sproutberry oil?
	StopDialogue();
	SetDialogueProgress(1);
	return true;
}
