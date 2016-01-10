// The village head.

#appendto Dialogue

public func Dlg_VillageHead_Init(object clonk)
{
	return true;
}

public func Dlg_VillageHead_1(object clonk)
{
	MessageBox("$DlgVillageHeadHowToAttack$", clonk, clonk);
	return true;
}

public func Dlg_VillageHead_2(object clonk)
{
	MessageBox("$DlgVillageHeadNoIdea$", clonk, dlg_target);
	if (!FindObject(Find_ID(Axe), Find_Container(clonk)))
		clonk->CreateContents(Axe);
	return true;
}

public func Dlg_VillageHead_3(object clonk)
{
	MessageBox("$DlgVillageHeadThanks$", clonk, clonk);
	return true;
}

public func Dlg_VillageHead_4(object clonk)
{
	MessageBox("$DlgVillageHeadGoodLuck$", clonk, dlg_target);
	StopDialogue();
	SetDialogueProgress(1);
	return true;
}

public func Dlg_VillageHead_Closed(object clonk)
{
	GameCall("OnHasTalkedToVillageHead", clonk);
	return true;
}
