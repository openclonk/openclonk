// An NPC on the lookout.

#appendto Dialogue

public func Dlg_Lookout_Init(object clonk)
{
	clonk->CreateContents(Blunderbuss);
	return true;
}

public func Dlg_Lookout_1(object clonk)
{
	MessageBox("$DlgLookoutNewAttack$", clonk, clonk);
	return true;
}

public func Dlg_Lookout_2(object clonk)
{
	MessageBox("$DlgLookoutNo$", clonk, dlg_target);
	StopDialogue();
	SetDialogueProgress(1);
	return true;
}