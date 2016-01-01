// A builder which tells you about the flagpole.

#appendto Dialogue

public func Dlg_Builder_Init(object clonk)
{
	return true;
}

public func Dlg_Builder_1(object clonk)
{
	MessageBox(Format("$DlgBuilderHello$", dlg_target->GetName()), clonk, dlg_target);
	return true;
}

public func Dlg_Builder_2(object clonk)
{
	MessageBox(Format("$DlgBuilderReply$", clonk->GetName()), clonk, clonk);
	return true;
}

public func Dlg_Builder_3(object clonk)
{
	MessageBox("$DlgBuilderFlagpole$", clonk, dlg_target);
	return true;
}

public func Dlg_Builder_4(object clonk)
{
	MessageBox("$DlgBuilderWhyHere$", clonk, clonk);
	return true;
}

public func Dlg_Builder_5(object clonk)
{
	MessageBox("$DlgBuilderConnect$", clonk, dlg_target);
	StopDialogue();
	SetDialogueProgress(1);
	return true;
}

public func Dlg_Builder_Closed(object clonk)
{
	GameCall("OnHasTalkedToBuilder", clonk);
	return true;
}
