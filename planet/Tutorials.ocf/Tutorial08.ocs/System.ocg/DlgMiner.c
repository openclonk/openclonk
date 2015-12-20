// A miner who just wants some food.

#appendto Dialogue

public func Dlg_Miner_Init(object clonk)
{
	AddEffect("IntMiner", clonk, 100, 5, this);
	var lamp = clonk->CreateContents(Lantern);
	lamp->TurnOn();
	clonk->CreateContents(Pickaxe);
	return true;
}

public func Dlg_Miner_1(object clonk)
{
	MessageBox(Format("$DlgMinerGreeting$", clonk->GetName()), clonk, clonk);
	return true;
}

public func Dlg_Miner_2(object clonk)
{
	MessageBox(Format("$DlgMinerYes$", dlg_target->GetName()), clonk, dlg_target);
	return true;
}

public func Dlg_Miner_3(object clonk)
{
	var food = FindObject(Find_Container(clonk), Find_Func("NutritionalValue"));
	if (food)
	{
		MessageBox(Format("$DlgMinerYesFood$", food.Name), clonk, clonk);
		food->Enter(dlg_target);
	}
	else
	{
		MessageBox("$DlgMinerNoFood$", clonk, clonk);
		StopDialogue();
		SetDialogueProgress(1);
	}
	return true;
}

public func Dlg_Miner_4(object clonk)
{
	var food = FindObject(Find_Container(dlg_target), Find_Func("NutritionalValue"));
	if (food)
		dlg_target->Eat(food);
	MessageBox("$DlgMinerEatFood$", clonk, dlg_target);
	return true;
}

public func Dlg_Miner_5(object clonk)
{
	MessageBox("$DlgMinerWayOut$", clonk, clonk);
	StopDialogue();
	SetDialogueProgress(4);
	return true;
}

public func Dlg_Miner_Closed(object clonk)
{
	return true;
}

public func FxIntMinerStart(object target, proplist effect, int temp)
{
	if (temp)
		return FX_OK;
	return FX_OK;
}

public func FxIntMinerTimer(object target, proplist effect, int time)
{
	return FX_OK;
}
