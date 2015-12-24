// An NPC next to the farming area.

#appendto Dialogue

public func Dlg_Farmer_Init(object clonk)
{
	return true;
}

public func Dlg_Farmer_1(object clonk)
{
	MessageBox("$DlgFarmerHello$", clonk, clonk);
	return true;
}

public func Dlg_Farmer_2(object clonk)
{
	MessageBox("$DlgFarmerYesSafe$", clonk, dlg_target);
	return true;
}

public func Dlg_Farmer_3(object clonk)
{
	MessageBox("$DlgFarmerRebuild$", clonk, clonk);
	StopDialogue();
	SetDialogueProgress(1);
	return true;
}
