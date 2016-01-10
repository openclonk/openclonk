// Dialogue for homeless guy due to the robbers.

#appendto Dialogue

public func Dlg_Homeless_Init(object clonk)
{
	return true;
}

public func Dlg_Homeless_1(object clonk)
{
	MessageBox("$DlgHomelessFood$", clonk, dlg_target);
	return true;
}

public func Dlg_Homeless_2(object clonk)
{
	var food = FindObject(Find_Container(clonk), Find_Func("NutritionalValue"));
	if (food)
	{
		MessageBox(Format("$DlgHomelessYes$", food.Name), clonk, clonk);
		food->Enter(dlg_target);
		SetDialogueProgress(4);
	}
	else
	{
		MessageBox("$DlgHomelessNo$", clonk, clonk);
	}
	return true;
}

public func Dlg_Homeless_3(object clonk)
{
	MessageBox("$DlgHomelessShame$", clonk, dlg_target);
	StopDialogue();
	SetDialogueProgress(1);
	return true;
}

public func Dlg_Homeless_4(object clonk)
{
	var food = FindObject(Find_Container(dlg_target), Find_Func("NutritionalValue"));
	if (food)
		dlg_target->Eat(food);
	clonk->CreateContents(Club);
	MessageBox("$DlgHomelessReward$", clonk, dlg_target);
	StopDialogue();
	SetDialogueProgress(1);
	return true;
}

public func Dlg_Homeless_Closed(object clonk)
{
	return true;
}
