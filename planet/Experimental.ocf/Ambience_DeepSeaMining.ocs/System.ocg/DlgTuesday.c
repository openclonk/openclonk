// NPC Tuesday: Sits on the island and does nothing
// (except giving some hints)

#appendto Dialogue

func Dlg_Tuesday_1(object clonk)
{
	MessageBox("$Tuesday1$", clonk, dlg_target);
	return true;
}

func Dlg_Tuesday_2(object clonk)
{
	var options = [["$TuesdayQCommunicator$", "Dlg_Tuesday_Communicator"], ["$TuesdayQGems$", "Dlg_Tuesday_Gems"], ["$TuesdayQFish$", "Dlg_Tuesday_Fish"], ["$TuesdayQWater$", "Dlg_Tuesday_Water"], ["$TuesdayQBye$", "StopDialogue()"]];
	MessageBox("", clonk, clonk, nil, false, options);
	SetDialogueProgress(1);
	return true;
}

func Dlg_Tuesday_Communicator(object clonk, q)
{
	SetDialogueProgress(2);
	return MessageBox("$TuesdayACommunicator$", clonk);
}

func Dlg_Tuesday_Gems(object clonk, q)
{
	SetDialogueProgress(2);
	return MessageBox("$TuesdayAGems$", clonk);
}

func Dlg_Tuesday_Fish(object clonk, q)
{
	SetDialogueProgress(2);
	return MessageBox("$TuesdayAFish$", clonk);
}

func Dlg_Tuesday_Water(object clonk, q)
{
	SetDialogueProgress(10);
	return MessageBox("$TuesdayAWater$", clonk);
}

func Dlg_Tuesday_10(object clonk)
{
	SetDialogueProgress(2);
	return MessageBox("$TuesdayAWater2$", clonk);
}
