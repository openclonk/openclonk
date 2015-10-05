#appendto Dialogue

/* Concierge dialogue */

func Dlg_Concierge_1(object clonk)
{
	MessageBox(Format("$Concierge1$", clonk->GetName()), clonk, dlg_target); // Welcome %s + Explanation
	StopDialogue();
	SetDialogueProgress(1);
	return true;
}