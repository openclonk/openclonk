#appendto Dialogue

/* Woody dialogue */

func Dlg_Woody_1(object clonk)
{
	MessageBox("$Woody1$", clonk, dlg_target); // I've injured my arm. can't chop wood.
	return true;
}

func Dlg_Woody_2(object clonk)
{
	MessageBox("$Woody2$", clonk, clonk); // can I help u?
	return true;
}

func Dlg_Woody_3(object clonk)
{
	MessageBox("$Woody3$", clonk, dlg_target); // chop tree plz
	return true;
}

func Dlg_Woody_4(object clonk)
{
	if (clonk->FindContents(Axe))
	{
		MessageBox("$Woody6$", clonk, clonk); // where r trees?
		SetDialogueProgress(6);
	}
	else
	{
		MessageBox("$Woody4$", clonk, clonk); // where is axe?
	}
	return true;
}

func Dlg_Woody_5(object clonk)
{
	MessageBox("$Woody5$", clonk, dlg_target); // look west @ mine
	StopDialogue();
	SetDialogueProgress(3);
	return true;
}

func Dlg_Woody_6(object clonk)
{
	MessageBox("$Woody7$", clonk, dlg_target); // look east @ forest
	StopDialogue();
	SetDialogueProgress(3);
	return true;
}

// post attack dialogue
func Dlg_Woody_100(object clonk)
{
	MessageBox("$Woody100$", clonk, dlg_target); // what can we do?
	StopDialogue();
	SetDialogueProgress(100);
	return true;
}

// Generic call on every dlg message of Woody
func Dlg_Woody(object clonk)
{
	// Stop walking.
	clonk->SetCommand("None");
	clonk->SetComDir(COMD_Stop);
	clonk->SetXDir();
	// Yield animation
	this.anim_continue_frame = FrameCounter() + 50;
	return false; // do call specific functions
}


/* NPC animations */

func Dlg_Woody_Init(object clonk)
{
	// Big axe!
	var axe = clonk->FindContents(Axe);
	if (!axe) axe = clonk->CreateContents(Axe);
	axe.GetCarryTransform = Dialogue.Inventory_GetCarryTransform;
	var h_scale = 2000;
	axe.ExtraTransform = Trans_Scale(h_scale, h_scale, h_scale);
	// Axe is heavy. Clonk moves slowly.
	clonk.ActMap = { Prototype = Clonk.ActMap, Walk = { Prototype = Clonk.ActMap.Walk } };
	clonk.ActMap.Walk.Speed /= 3;
	clonk->SetAction("Walk");
	// Walking about animation
	AddEffect("WoodyWalking", clonk, 1, 40, this);
	return true;
}

func FxWoodyWalkingTimer(object c, proplist fx, int time)
{
	if (FrameCounter() < this.anim_continue_frame) return FX_OK;
	if (!Random(2) && GetPlayerCount())
	{
		// Move between places (only if players are joined so Woody stays in place for object saving)
		c->SetCommand("MoveTo", nil, 750 + Random(50), 366);
	}
	return FX_OK;
}

