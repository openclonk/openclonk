// The village head is in the right mine.

#appendto Dialogue

public func Dlg_VillageHead_Init(object clonk)
{
	var lantern = clonk->CreateContents(Lantern);
	lantern->TurnOn();
	AddEffect("IntVillageHead", clonk, 100, 5, this);
	return true;
}

public func Dlg_VillageHead_1(object clonk)
{
	MessageBox("$DlgVillageHeadThank$", clonk, dlg_target);
	return true;
}

public func Dlg_VillageHead_2(object clonk)
{
	MessageBox("$DlgVillageHeadHelp$", clonk, clonk);
	return true;
}

public func Dlg_VillageHead_3(object clonk)
{
	MessageBox("$DlgVillageHeadTools$", clonk, dlg_target);
	return true;
}

public func Dlg_VillageHead_4(object clonk)
{
	MessageBox("$DlgVillageHeadProblem$", clonk, clonk);
	return true;
}

public func Dlg_VillageHead_5(object clonk)
{
	MessageBox("$DlgVillageHeadGap$", clonk, dlg_target);
	return true;
}

public func Dlg_VillageHead_6(object clonk)
{
	MessageBox("$DlgVillageHeadFix$", clonk, clonk);
	return true;
}

public func Dlg_VillageHead_7(object clonk)
{
	MessageBox("$DlgVillageHeadPickaxe$", clonk, dlg_target);
	return true;
}

public func Dlg_VillageHead_8(object clonk)
{
	MessageBox("$DlgVillageHeadYes$", clonk, clonk);
	StopDialogue();
	SetDialogueProgress(1);
	return true;
}

public func Dlg_VillageHead_9(object clonk)
{
	var foundry = FindObject(Find_ID(Foundry));
	if (!foundry || ObjectCount(Find_ID(Ore), Find_Container(foundry)) < 3)
	{
		MessageBox("$DlgVillageHeadPutOre$", clonk, dlg_target);
		StopDialogue();
		SetDialogueProgress(9);
		return true;
	}
	this.has_finished = true;
	MessageBox("$DlgVillageHeadThanks$", clonk, dlg_target);
	StopDialogue();
	SetDialogueProgress(9);
	return true;
}

public func Dlg_VillageHead_Closed(object clonk)
{
	GameCall("OnHasTalkedToVillageHead", clonk);
	if (this.has_finished)
		GameCall("OnHasTalkedToVillageHeadFinished", clonk);
	return true;
}

public func FxIntVillageHeadStart(object target, proplist effect, bool temp)
{
	if (temp)
		return FX_OK;
	effect.move_up = false;
	effect.state = 0;
	return FX_OK;
}

public func FxIntVillageHeadTimer(object target, proplist effect, int time)
{
	if (!effect.move_up)
		return FX_OK;
		
	if (effect.state == 0 && !target->GetCommand())
	{
		if (target->GetX() < 626)
			effect.state = 1;
		else
			target->SetCommand("MoveTo", nil, 620, 650);
		return FX_OK;
	}
	
	if (effect.state == 1)
	{
		var case = FindObject(Find_ID(ElevatorCase));
		if (case->GetY() > 644)
			effect.state = 2;
		else if (case->Ready(target))
			case->CallCase(target);
		return FX_OK;
	}
	
	if (effect.state == 2)
	{
		var case = FindObject(Find_ID(ElevatorCase));
		if (case->GetY() < 382)
			effect.state = 3;
		else
		{
			if (!target->GetCommand())
				target->SetCommand("Grab", case);
			case->ControlUp(target);
		}
		return FX_OK;
	}
	
	if (effect.state == 3)
	{
		target->SetCommand("UnGrab");
		target->AppendCommand("MoveTo", nil, target->GetX() - 40, target->GetY());
		effect.state = 4;
		return FX_OK;
	}
	
	return FX_OK;
}
