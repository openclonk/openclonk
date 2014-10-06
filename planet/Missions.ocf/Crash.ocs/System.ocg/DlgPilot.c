// NPC Pilot: The one that can really fly the airplane.

#appendto Dialogue


func Dlg_Pilot_1(object clonk)
{
	MessageBox("$MsgCrashedPlane$", clonk);
	return true;
}

func Dlg_Pilot_2(object clonk)
{
	var options = [["$PyritQPlane$", "Dlg_Pilot_Plane"], ["$PyritQLake$", "Dlg_Pilot_Lake"], ["$PyritQDone$", "StopDialogue()"]];
	MessageBox("$AnsCrashedPlane$", clonk, clonk, nil, false, options);	
	SetDialogueProgress(1);
	return true;
}

func Dlg_Pilot_Plane(object clonk, q)
{
	MessageBox("$PyritAPlane$", clonk);
	return StopDialogue();
}

func Dlg_Pilot_Lake(object clonk, q)
{
	MessageBox("$PyritALake$", clonk);
	return StopDialogue();
}
