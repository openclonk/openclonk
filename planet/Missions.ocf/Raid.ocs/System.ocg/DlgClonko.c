#appendto Dialogue

/* Clonko Dialogue */

func Dlg_Clonko_1(object clonk)
{
	MessageBox("$Clonko1$", clonk, dlg_target); // hello
	return true;
}

func Dlg_Clonko_2(object clonk)
{
	MessageBox("$Clonko2$", clonk, clonk); // where is Pyrit?
	return true;
}

func Dlg_Clonko_3(object clonk)
{
	MessageBox("$Clonko3$", clonk, dlg_target); // red guy 2 the left
	return true;
}

func Dlg_Clonko_4(object clonk)
{
	MessageBox("$Clonko4$", clonk, clonk); // thx
	StopDialogue();
	SetDialogueProgress(1);
	return true;
}


/* After speaking to Pyrit: Ask for oil */

func Dlg_Clonko_100(object clonk)
{
	MessageBox("$Clonko100$", clonk, clonk); // where is oil?
	return true;
}

func Dlg_Clonko_101(object clonk)
{
	MessageBox("$Clonko101$", clonk, dlg_target); // oil not allowed
	return true;
}

func Dlg_Clonko_102(object clonk)
{
	MessageBox("$Clonko102$", clonk, clonk); // nothing hidden?
	return true;
}

func Dlg_Clonko_103(object clonk)
{
	MessageBox("$Clonko103$", clonk, dlg_target); // nope
	return true;
}

func Dlg_Clonko_104(object clonk)
{
	MessageBox("$Clonko104$", clonk, dlg_target); // but rumours...
	return true;
}

func Dlg_Clonko_105(object clonk)
{
	MessageBox("$Clonko105$", clonk, clonk); // plz elaborate
	return true;
}

func Dlg_Clonko_106(object clonk)
{
	MessageBox("$Clonko106$", clonk, dlg_target); // underground oil lake
	g_clonko_spoken = true;
	Dialogue->FindByTarget(npc_dora)->AddAttention();
	return true;
}

func Dlg_Clonko_107(object clonk)
{
	// if player was already down there, skip to complaint
	if (g_dora_spoken) return Dlg_Clonko_200(clonk);
	MessageBox("$Clonko107$", clonk, clonk); // how 2 get there?
	return true;
}

func Dlg_Clonko_108(object clonk)
{
	MessageBox("$Clonko108$", clonk, dlg_target); // check below this town
	return true;
}

func Dlg_Clonko_109(object clonk)
{
	MessageBox("$Clonko109$", clonk, clonk); // on my way
	StopDialogue();
	SetDialogueProgress(107);
	return true;
}


/* Dialogue after speaking to Dora */

func Dlg_Clonko_200(object clonk)
{
	MessageBox("$Clonko200$", clonk, clonk); // there is no lake!
	SetDialogueProgress(201);
	return true;
}

func Dlg_Clonko_201(object clonk)
{
	MessageBox("$Clonko201$", clonk, dlg_target); // tough luck.
	StopDialogue();
	SetDialogueProgress(106);
	return true;
}
