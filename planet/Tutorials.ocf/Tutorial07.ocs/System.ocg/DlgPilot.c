// A pilot which tells about the sky islands.

#appendto Dialogue

public func Dlg_Pilot_Init(object clonk)
{
	AddEffect("IntPilot", clonk, 100, 5, this);
	return true;
}

public func Dlg_Pilot_1(object clonk)
{
	MessageBox(Format("$DlgPilotHello$", dlg_target->GetName()), clonk, dlg_target);
	return true;
}

public func Dlg_Pilot_2(object clonk)
{
	MessageBox("$DlgPilotReply$", clonk, clonk);
	return true;
}

public func Dlg_Pilot_3(object clonk)
{
	MessageBox("$DlgPilotBats$", clonk, dlg_target);
	return true;
}

public func Dlg_Pilot_4(object clonk)
{
	MessageBox("$DlgPilotAgainstBats$", clonk, clonk);
	return true;
}

public func Dlg_Pilot_5(object clonk)
{
	MessageBox("$DlgPilotBowArrow$", clonk, dlg_target);
	return true;
}

public func Dlg_Pilot_6(object clonk)
{
	MessageBox("$DlgPilotHaveBow$", clonk, clonk);
	return true;
}

public func Dlg_Pilot_7(object clonk)
{
	MessageBox("$DlgPilotArmory$", clonk, dlg_target);
	StopDialogue();
	SetDialogueProgress(1);
	return true;
}

public func Dlg_Pilot_Closed(object clonk)
{
	GameCall("OnHasTalkedToFireman", clonk);
	return true;
}

public func FxIntPilotStart(object target, proplist effect)
{
	return FX_OK;
}

public func FxIntPilotTimer(object target, proplist effect, int time)
{

	return FX_OK;
}