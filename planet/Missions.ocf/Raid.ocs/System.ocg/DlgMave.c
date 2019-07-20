#appendto Dialogue

/* Mave Dialogue */

func Dlg_Mave_1(object clonk)
{
	MessageBox(Format("$Mave1$", clonk->GetName()), clonk, dlg_target); // have u heard rumbling
	return true;
}

func Dlg_Mave_2(object clonk)
{
	MessageBox("$Mave2$", clonk, clonk); // village down. plz help
	return true;
}

func Dlg_Mave_3(object clonk)
{
	MessageBox("$Mave3$", clonk, dlg_target); // no
	return true;
}

func Dlg_Mave_4(object clonk)
{
	MessageBox("$Mave4$", clonk, clonk); // help me go east?
	return true;
}

func Dlg_Mave_5(object clonk)
{
	MessageBox("$Mave5$", clonk, dlg_target); // caves unusable
	return true;
}

func Dlg_Mave_6(object clonk)
{
	MessageBox("$Mave6$", clonk, dlg_target); // u must cross lake
	return true;
}

func Dlg_Mave_7(object clonk)
{
var options = [["$Mave7$", "Dlg_Mave_Loam"], ["$Mave12$", "Dlg_Mave_Elev"], ["$MaveBye$", "StopDialogue()"]];
	MessageBox("", clonk, clonk, nil, false, options); // how cross lake? where elevator?
	return true;
}

func Dlg_Mave_Loam(object clonk)
{
	MessageBox("$Mave8$", clonk, dlg_target); // build loam
	SetDialogueProgress(9);
	return true;
}

func Dlg_Mave_9(object clonk)
{
	MessageBox("$Mave9$", clonk, clonk); // how?
	return true;
}

func Dlg_Mave_10(object clonk)
{
	MessageBox("$Mave10$", clonk, dlg_target); // bucket + barrel
	return true;
}

func Dlg_Mave_11(object clonk)
{
	MessageBox("$Mave11$", clonk, dlg_target); // tools in foundry
	StopDialogue();
	SetDialogueProgress(7);
	return true;
}

func Dlg_Mave_Elev(object clonk)
{
	MessageBox("$Mave13$", clonk, dlg_target); // the elevator leads to the oil chamber
	StopDialogue();
	SetDialogueProgress(7);
	return true;
}


/* Dialogue after speaking to Pyrit - ask for oil */

func Dlg_Mave_100(object clonk)
{
	SetBroadcast(true);
	MessageBox("$Mave100$", clonk, clonk); // where is oil?
	return true;
}

func Dlg_Mave_101(object clonk)
{
	MessageBox("$Mave101$", clonk, dlg_target); // oil not allowed
	return true;
}

func Dlg_Mave_102(object clonk)
{
	MessageBox("$Mave102$", clonk, clonk); // i want kill king
	return true;
}

func Dlg_Mave_103(object clonk)
{
	MessageBox("$Mave103$", clonk, dlg_target); // good. i could help
	return true;
}

func Dlg_Mave_104(object clonk)
{
	MessageBox("$Mave104$", clonk, clonk); // yes please
	return true;
}

func Dlg_Mave_105(object clonk)
{
	MessageBox("$Mave105$", clonk, dlg_target); // oil is in chamber
	return true;
}

func Dlg_Mave_106(object clonk)
{
	MessageBox("$Mave106$", clonk, dlg_target); // chamber is down the shaft
	return true;
}

func Dlg_Mave_107(object clonk)
{
	MessageBox("$Mave107$", clonk, dlg_target); // u need key
	return true;
}

func Dlg_Mave_108(object clonk)
{
	MessageBox("$Mave108$", clonk, clonk); // k key plz
	return true;
}

func Dlg_Mave_109(object clonk)
{
	MessageBox("$Mave109$", clonk, dlg_target); // i give key 4 gold bar
	g_mave_oil_spoken = true; // this updates Pyrit's dialogue - but not enough to warrant a new attention marker
	return true;
}

func Dlg_Mave_110(object clonk)
{
	// gold bar found?
	var gold_bar = clonk->FindContents(GoldBar);
	if (!gold_bar) gold_bar = FindObject(Find_AtRect(-30,-30, 60, 60), Find_ID(GoldBar));
	if (gold_bar) return Dlg_Mave_200(clonk, gold_bar);
	// not found yet
	MessageBox("$Mave110$", clonk, clonk); // where is gold?
	return true;
}

func Dlg_Mave_111(object clonk)
{
	MessageBox("$Mave111$", clonk, dlg_target); // no idea
	return true;
}

func Dlg_Mave_112(object clonk)
{
	MessageBox("$Mave112$", clonk, dlg_target); // bring nuggets 2 foundry 4 gold bar
	SetBroadcast(false);
	StopDialogue();
	SetDialogueProgress(109);
	return true;
}


/* Dialogue after handing over gold bar */

func Dlg_Mave_200(object clonk, object gold_bar)
{
	gold_bar->RemoveObject();
	g_allow_gold_sale = true;
	SetBroadcast(true);
	MessageBox("$Mave200$", clonk, clonk); // here is gold bar
	SetDialogueProgress(201);
	return true;
}

func Dlg_Mave_201(object clonk)
{
	MessageBox("$Mave201$", clonk, dlg_target); // here is key
	g_got_maves_key = true;
	clonk->CreateContents(Key);
	return true;
}

func Dlg_Mave_202(object clonk)
{
	MessageBox("$Mave202$", clonk, clonk); // where is chamber?
	return true;
}

func Dlg_Mave_203(object clonk)
{
	MessageBox("$Mave203$", clonk, dlg_target); // below lava
	SetBroadcast(false);
	StopDialogue();
	SetDialogueProgress(202);
	return true;
}
