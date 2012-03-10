// NPC Pilot: The one that can really fly the airplane.

#appendto Dialogue


private func Dlg_Pilot_1(object clonk)
{
	MessageBox("$MsgCrashedPlane$", clonk);
	return;
}

private func Dlg_Pilot_2(object clonk)
{
	MessageBox("$AnsCrashedPlane$", clonk, clonk);
	SetDialogueStatus(DLG_Status_Stop);
	return;
}

