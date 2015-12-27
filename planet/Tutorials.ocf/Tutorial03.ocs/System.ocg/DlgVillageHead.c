// The village head is in the right mine.

#appendto Dialogue

public func Dlg_VillageHead_Init(object clonk)
{
	return true;
}

public func Dlg_VillageHead_1(object clonk)
{
	MessageBox("$DlgVillageHeadHello$", clonk, dlg_target);
	return true;
}

public func Dlg_VillageHead_2(object clonk)
{
	MessageBox("$DlgVillageHeadHere$", clonk, clonk);
	return true;
}

public func Dlg_VillageHead_3(object clonk)
{
	MessageBox("$DlgVillageHeadOverview$", clonk, dlg_target);
	StopDialogue();
	SetDialogueProgress(1);
	return true;
}
