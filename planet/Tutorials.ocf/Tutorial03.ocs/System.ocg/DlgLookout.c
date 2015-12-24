// An NPC on the lookout.

#appendto Dialogue

public func Dlg_Lookout_Init(object clonk)
{
	clonk->CreateContents(Musket);
	return true;
}

public func Dlg_Lookout_1(object clonk)
{
	MessageBox("$DlgLookoutHello$", clonk, clonk);
	return true;
}

public func Dlg_Lookout_2(object clonk)
{
	MessageBox("$DlgLookoutProtecting$", clonk, dlg_target);
	return true;
}

public func Dlg_Lookout_3(object clonk)
{
	MessageBox("$DlgLookoutMusket$", clonk, clonk);
	return true;
}

public func Dlg_Lookout_4(object clonk)
{
	MessageBox("$DlgLookoutNoChance$", clonk, dlg_target);
	return true;
}

public func Dlg_Lookout_5(object clonk)
{
	MessageBox("$DlgLookoutNoShow$", clonk, clonk);
	StopDialogue();
	SetDialogueProgress(1);
	return true;
}
