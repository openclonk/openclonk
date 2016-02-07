// The village head.

#appendto Dialogue

public func Dlg_VillageHead_Init(object clonk)
{
	return true;
}

public func Dlg_VillageHead_1(object clonk)
{
	MessageBox("$DlgVillageHeadGrateful$", clonk, dlg_target);
	return true;
}

public func Dlg_VillageHead_2(object clonk)
{
	MessageBox("$DlgVillageHeadWhatToDo$", clonk, clonk);
	return true;
}

public func Dlg_VillageHead_3(object clonk)
{
	MessageBox("$DlgVillageHeadPower$", clonk, dlg_target);
	return true;
}

public func Dlg_VillageHead_4(object clonk)
{
	MessageBox("$DlgVillageHeadIWill$", clonk, clonk);
	return true;
}

public func Dlg_VillageHead_5(object clonk)
{
	MessageBox("$DlgVillageHeadAfterwards$", clonk, dlg_target);
	StopDialogue();
	SetDialogueProgress(1);
	return true;
}

public func Dlg_VillageHead_Closed(object clonk)
{
	return true;
}
