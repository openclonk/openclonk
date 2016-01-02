// A fireman which puts out the fires around the village.

#appendto Dialogue

public func Dlg_Fireman_Init(object clonk)
{
	return true;
}

public func Dlg_Fireman_1(object clonk)
{
	MessageBox("$DlgFiremanWhereBarrel$", clonk, clonk);
	return true;
}

public func Dlg_Fireman_2(object clonk)
{
	MessageBox("$DlgFiremanFoundry$", clonk, dlg_target);
	return true;
}

public func Dlg_Fireman_3(object clonk)
{
	MessageBox("$DlgFiremanWhereWater$", clonk, clonk);
	return true;
}

public func Dlg_Fireman_4(object clonk)
{
	MessageBox("$DlgFiremanDive$", clonk, dlg_target);
	StopDialogue();
	SetDialogueProgress(1);
	return true;
}

public func Dlg_Fireman_Closed(object clonk)
{
	return true;
}
