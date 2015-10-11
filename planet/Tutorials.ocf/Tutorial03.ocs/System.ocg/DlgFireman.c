// A fireman which puts out the fires around the village.

#appendto Dialogue

public func Dlg_Fireman_Init(object clonk)
{
	clonk->Message("<c ff0000>Fire!</c> Out of my way!");
	AddEffect("IntFireman", clonk, 100, 5, this);
	return true;
}

public func Dlg_Fireman_1(object clonk)
{
	MessageBox("$DlgFiremanLastFire$", clonk, dlg_target);
	return true;
}

public func Dlg_Fireman_2(object clonk)
{
	MessageBox("$DlgFiremanGoodJob$", clonk, clonk);
	return true;
}

public func Dlg_Fireman_3(object clonk)
{
	MessageBox("$DlgFiremanEvilGuys$", clonk, dlg_target);
	return true;
}

public func Dlg_Fireman_4(object clonk)
{
	MessageBox("$DlgFiremanFurryFriend$", clonk, clonk);
	return true;
}

public func Dlg_Fireman_5(object clonk)
{
	MessageBox("$DlgFiremanFlagpole$", clonk, dlg_target);
	StopDialogue();
	SetDialogueProgress(5);
	return true;
}

public func Dlg_Fireman_Closed(object clonk)
{
	GameCall("OnHasTalkedToFireman", clonk);
	return true;
}

public func FxIntFiremanStart(object target, proplist effect)
{
	return FX_OK;
}

public func FxIntFiremanTimer(object target, proplist effect, int time)
{
	if (time == 20)
		target->SetCommand("MoveTo", nil, 320, 348);
	
	if (!target->GetCommand() && time > 50)
	{
		target->SetDir(DIR_Right);
		target->Contents(0)->ControlUse(target, 10, -2);		
		return FX_Execute_Kill;
	}
	return FX_OK;
}