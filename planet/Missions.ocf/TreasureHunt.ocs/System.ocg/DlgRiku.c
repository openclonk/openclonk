#appendto Dialogue

/* Riku dialogue */

func Dlg_Riku_1(object clonk)
{
	if (g_treasure_collected) return CallDialogue(clonk, 1, "Gem");
	MessageBox("$Riku1$", clonk, dlg_target); // looking 4 gold?
	return true;
}

func Dlg_Riku_2(object clonk)
{
	// This is unlikely, but possible: Player entered the caves without speaking to Dagobert
	if (!g_got_gem_task) return CallDialogue(clonk, 1, "Oil");
	MessageBox("$Riku2$", clonk, clonk); // looking 4 gem
	return true;
}

func Dlg_Riku_3(object clonk)
{
	MessageBox("$Riku3$", clonk, dlg_target); // look in treasure chamber
	return true;
}

func Dlg_Riku_4(object clonk)
{
	MessageBox("$Riku4$", clonk, clonk); // how 2 open door?
	return true;
}

func Dlg_Riku_5(object clonk)
{
	MessageBox("$Riku5$", clonk, dlg_target); // switches hidden in cave
	return true;
}

func Dlg_Riku_6(object clonk)
{
	MessageBox("$Riku6$", clonk, clonk); // u search
	return true;
}

func Dlg_Riku_7(object clonk)
{
	MessageBox("$Riku7$", clonk, dlg_target); // no u do
	return true;
}

func Dlg_Riku_8(object clonk)
{
	MessageBox("$Riku8$", clonk, clonk); // ok
	SetDialogueProgress(1);
	return StopDialogue();
}

func Dlg_Riku_Oil1(object clonk)
{
	MessageBox("$RikuOil1$", clonk, clonk); // looking 4 oil
	return true;
}

func Dlg_Riku_Oil2(object clonk)
{
	MessageBox("$RikuOil2$", clonk, dlg_target); // ask dagobert
	SetDialogueProgress(1);
	return StopDialogue();
}

func Dlg_Riku_Gem1(object clonk)
{
	MessageBox("$RikuGem1$", clonk, clonk); // i found gem
	return true;
}

func Dlg_Riku_Gem2(object clonk)
{
	MessageBox("$RikuGem2$", clonk, dlg_target); // k i keep rest
	SetDialogueProgress(1);
	return StopDialogue();
}
