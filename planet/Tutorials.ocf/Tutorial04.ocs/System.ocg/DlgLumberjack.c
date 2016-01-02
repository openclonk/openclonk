// A lumberjack which tells you how to use the sawmill.

#appendto Dialogue

public func Dlg_Lumberjack_Init(object clonk)
{
	clonk->CreateContents(Axe);
	return true;
}

public func Dlg_Lumberjack_1(object clonk)
{
	MessageBox("$DlgLumberjackForest$", clonk, clonk);
	return true;
}

public func Dlg_Lumberjack_2(object clonk)
{
	MessageBox("$DlgLumberjackCutDown$", clonk, dlg_target);
	return true;
}

public func Dlg_Lumberjack_3(object clonk)
{
	MessageBox("$DlgLumberjackNoWood$", clonk, clonk);
	return true;
}

public func Dlg_Lumberjack_4(object clonk)
{
	MessageBox("$DlgLumberjackSupply$", clonk, dlg_target);
	StopDialogue();
	SetDialogueProgress(1);
	return true;
}

public func Dlg_Lumberjack_Closed(object clonk)
{
	return true;
}