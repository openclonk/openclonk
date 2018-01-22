#appendto Dialogue

/* Dagobert dialogue */

func Dlg_Dagobert_1(object clonk)
{
	var gem = FindObject(Find_ID(GemOfPower), Find_AtRect(-20,-20,20,20));
	if (g_got_oil)
	{
		MessageBox("$Dagobert1B$", clonk, dlg_target); // beautiful gem
		SetDialogueProgress(1);
		StopDialogue();
	}
	else if (gem)
	{
		gem->Enter(dlg_target);
		MessageBox("$DagobertGem0$", clonk, clonk); // here is gem
		SetDialogueProgress(1, "Gem");
	}
	else
	{
		MessageBox("$Dagobert1$", clonk, dlg_target); // gold
	}
	return true;
}

func Dlg_Dagobert_2(object clonk)
{
	var options = [["$DagobertGoldQ$", "#Gold"], ["$DagobertOilQ$", "#Oil"], ["$DagobertCaveQ$", "#Cave"], ["$DagobertBye$", "StopDialogue()"]];
	MessageBox("", clonk, clonk, nil, false, options);
	SetDialogueProgress(1);
	return true;
}

func Dlg_Dagobert_Gold1(object clonk)
{
	MessageBox("$DagobertGold1$", clonk, dlg_target); // treasures b plenty
	return true;
}

func Dlg_Dagobert_Gold2(object clonk)
{
	MessageBox("$DagobertGold2$", clonk, clonk); // y u not go?
	return true;
}

func Dlg_Dagobert_Gold3(object clonk)
{
	MessageBox("$DagobertGold3$", clonk, dlg_target); // lava & water
	return true;
}

func Dlg_Dagobert_Gold4(object clonk)
{
	MessageBox("$DagobertGold4$", clonk, clonk); // dive?
	return true;
}

func Dlg_Dagobert_Gold5(object clonk)
{
	MessageBox("$DagobertGold5$", clonk, dlg_target); // can't
	SetDialogueProgress(2);
	return true;
}

func Dlg_Dagobert_Oil1(object clonk)
{
	if (g_got_oil)
	{
		MessageBox("$DagobertOil1C$", clonk, dlg_target); // oil for gem
		SetDialogueProgress(2);
	}
	else if (g_got_gem_task)
	{
		MessageBox("$DagobertOil1B$", clonk, dlg_target); // u already got oil
		SetDialogueProgress(2);
	}
	else
	{
		MessageBox("$DagobertOil1$", clonk, dlg_target); // oil not allowed
	}
	return true;
}

func Dlg_Dagobert_Oil2(object clonk)
{
	MessageBox("$DagobertOil2$", clonk, clonk); // i'll kill harx
	return true;
}

func Dlg_Dagobert_Oil3(object clonk)
{
	MessageBox("$DagobertOil3$", clonk, dlg_target); // good. i'll help
	return true;
}

func Dlg_Dagobert_Oil4(object clonk)
{
	MessageBox("$DagobertOil4$", clonk, dlg_target); // hidden treasure
	return true;
}

func Dlg_Dagobert_Oil5(object clonk)
{
	MessageBox("$DagobertOil5$", clonk, dlg_target); // gem of power
	return true;
}

func Dlg_Dagobert_Oil6(object clonk)
{
	MessageBox("$DagobertOil6$", clonk, dlg_target); // gem i want
	return true;
}

func Dlg_Dagobert_Oil7(object clonk)
{
	MessageBox("$DagobertOil7$", clonk, clonk); // ok
	g_got_gem_task = true;
	if (g_goal) g_goal->OnGotGemTask();
	SetDialogueProgress(2);
	return true;
}

func Dlg_Dagobert_Cave1(object clonk)
{
	MessageBox("$DagobertCave1$", clonk, dlg_target); // lava blocks entry. water below
	return true;
}

func Dlg_Dagobert_Cave2(object clonk)
{
	MessageBox("$DagobertCave2$", clonk, dlg_target); // reroute water
	return true;
}

func Dlg_Dagobert_Cave3(object clonk)
{
	MessageBox("$DagobertCave3$", clonk, clonk); // whats inside?
	return true;
}

func Dlg_Dagobert_Cave4(object clonk)
{
	MessageBox("$DagobertCave4$", clonk, dlg_target); // rough terrain
	return true;
}

func Dlg_Dagobert_Cave5(object clonk)
{
	MessageBox("$DagobertCave5$", clonk, dlg_target); // u might find tools
	SetDialogueProgress(2);
	return true;
}

func Dlg_Dagobert_Gem1(object clonk)
{
	MessageBox("$DagobertGem1$", clonk, dlg_target); // incredible
	return true;
}

func Dlg_Dagobert_Gem2(object clonk)
{
	MessageBox("$DagobertGem2$", clonk, dlg_target); // take oil
	var barrel = CreateObjectAbove(MetalBarrel);
	if (barrel)
	{
		barrel->SetXDir(-15);
		barrel->SetYDir(-20);
		barrel->PutLiquid("Oil");
	}
	g_got_oil = true;
	if (g_goal) g_goal->OnTreasureSold();
	return true;
}

func Dlg_Dagobert_Gem3(object clonk)
{
	MessageBox("$DagobertGem3$", clonk, dlg_target); // thanks
	SetDialogueProgress(1);
	return StopDialogue();
}

func Dlg_Dagobert_Init(object clonk)
{
	// Localized name
	clonk->SetName("$DagobertName$");
	return true;
}
