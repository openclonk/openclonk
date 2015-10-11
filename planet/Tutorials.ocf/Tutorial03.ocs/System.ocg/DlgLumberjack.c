// A lumberjack which tells you how to use the sawmill.

#appendto Dialogue

public func Dlg_Lumberjack_Init(object clonk)
{
	return true;
}

public func Dlg_Lumberjack_1(object clonk)
{
	MessageBox(Format("$DlgLumberjackHello$", dlg_target->GetName()), clonk, dlg_target);
	return true;
}

public func Dlg_Lumberjack_2(object clonk)
{
	MessageBox(Format("$DlgLumberjackReply$", clonk->GetName()), clonk, clonk);
	return true;
}

public func Dlg_Lumberjack_3(object clonk)
{
	MessageBox("$DlgLumberjackSawmill$", clonk, dlg_target);
	return true;
}

public func Dlg_Lumberjack_4(object clonk)
{
	MessageBox("$DlgLumberjackRock$", clonk, clonk);
	StopDialogue();
	SetDialogueProgress(1);
	return true;
}

public func Dlg_Lumberjack_5(object clonk)
{
	MessageBox("$DlgLumberjackWellDone$", clonk, dlg_target);
	return true;
}

public func Dlg_Lumberjack_6(object clonk)
{
	MessageBox("$DlgLumberjackFavor$", clonk, clonk);
	return true;
}

public func Dlg_Lumberjack_7(object clonk)
{
	MessageBox("$DlgLumberjackMines$", clonk, dlg_target);
	return true;
}

public func Dlg_Lumberjack_8(object clonk)
{
	MessageBox("$DlgLumberjackLook$", clonk, clonk);
	StopDialogue();
	SetDialogueProgress(5);
	return true;
}

public func Dlg_Lumberjack_Closed(object clonk)
{
	GameCall("OnHasTalkedToLumberjack", clonk);
	return true;
}