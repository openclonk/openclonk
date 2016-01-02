// A builder npc.

#appendto Dialogue

public func Dlg_Builder_Init(object clonk)
{
	return true;
}

public func Dlg_Builder_1(object clonk)
{
	MessageBox("$DlgBuilderThanks$", clonk, dlg_target);
	return true;
}

public func Dlg_Builder_2(object clonk)
{
	MessageBox("$DlgBuilderNoProblem$", clonk, clonk);
	return true;
}

public func Dlg_Builder_3(object clonk)
{
	MessageBox("$DlgBuilderPowerConnection$", clonk, dlg_target);
	StopDialogue();
	SetDialogueProgress(1);
	return true;
}

public func Dlg_Builder_Closed(object clonk)
{
	return true;
}