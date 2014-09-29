#appendto Dialogue

/* Dora Dialogue */

func Dlg_Dora_1(object clonk)
{
	MessageBox("$Dora1$", clonk, dlg_target); // hello
	return true;
}

func Dlg_Dora_2(object clonk)
{
	// Didn't speak to Pyrit before? Searching for Pyrit then.
	if (!g_pyrit_spoken) return Dlg_Dora_200(clonk);
	// Spoke to Pyrit (seeks oil) but not to Clonko (doesn't know rumous)?
	if (!g_clonko_spoken) return Dlg_Dora_100(clonk);
	// Regular dialogue: Heard about oil lake rumour
	MessageBox("$Dora2$", clonk, clonk); // where is oil lake?
	return true;
}

func Dlg_Dora_3(object clonk)
{
	MessageBox("$Dora3$", clonk, dlg_target); // no lake
	return true;
}

func Dlg_Dora_4(object clonk)
{
	MessageBox("$Dora4$", clonk, clonk); // but rumours?
	return true;
}

func Dlg_Dora_5(object clonk)
{
	MessageBox("$Dora5$", clonk, dlg_target); // aren't true
	g_dora_spoken = true; // updates Clonkos dialogue
	Dialogue->FindByTarget(npc_clonko)->AddAttention();
	SetDialogueProgress(7);
	return true;
}

func Dlg_Dora_6(object clonk)
{
	MessageBox("$Dora6$", clonk, dlg_target); // rumours aren't true
	return true;
}

func Dlg_Dora_7(object clonk)
{
	MessageBox("$Dora7$", clonk, clonk); // where can i find oil then?
	return true;
}


func Dlg_Dora_8(object clonk)
{
	MessageBox("$Dora8$", clonk, dlg_target); // ask Newton
	StopDialogue();
	SetDialogueProgress(6);
	return true;
}



/* Dialogue if looking for oil but not knowing about rumour */

func Dlg_Dora_100(object clonk)
{
	MessageBox("$Dora100$", clonk, clonk); // where is oil?
	SetDialogueProgress(101);
	return true;
}

func Dlg_Dora_101(object clonk)
{
	MessageBox("$Dora101$", clonk, dlg_target); // no lake. despite rumours
	return true;
}

func Dlg_Dora_102(object clonk)
{
	MessageBox("$Dora102$", clonk, clonk); // but...?
	return true;
}

func Dlg_Dora_103(object clonk)
{
	MessageBox("$Dora103$", clonk, dlg_target); // rumours aren't true
	g_dora_spoken = true; // updates Clonkos dialogue
	Dialogue->FindByTarget(npc_clonko)->AddAttention();
	StopDialogue();
	SetDialogueProgress(103);
	return true;
}


/* Dialogue if still looking for Pyrit */

func Dlg_Dora_200(object clonk)
{
	MessageBox("$Dora200$", clonk, clonk); // where is Pyrit?
	SetDialogueProgress(201);
	return true;
}

func Dlg_Dora_201(object clonk)
{
	MessageBox("$Dora201$", clonk, dlg_target); // above
	StopDialogue();
	SetDialogueProgress(1);
	return true;
}
