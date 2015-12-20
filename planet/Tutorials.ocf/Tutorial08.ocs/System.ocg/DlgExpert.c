// An explosive expert who tells about dynamite.

#appendto Dialogue

public func Dlg_Expert_Init(object clonk)
{
	AddEffect("IntExpert", clonk, 100, 5, this);
	clonk->CreateContents(DynamiteBox);
	return true;
}

public func Dlg_Expert_1(object clonk)
{
	MessageBox("$DlgExpertHello$", clonk, clonk);
	return true;
}

public func Dlg_Expert_2(object clonk)
{
	MessageBox("$DlgExpertNoHelp$", clonk, dlg_target);
	return true;
}

public func Dlg_Expert_3(object clonk)
{
	MessageBox("$DlgExpertWhatExplosives$", clonk, clonk);
	return true;
}

public func Dlg_Expert_4(object clonk)
{
	MessageBox("$DlgExpertUseExplosives$", clonk, dlg_target);
	return true;
}

public func Dlg_Expert_5(object clonk)
{
	MessageBox("$DlgExpertThanks$", clonk, clonk);
	StopDialogue();
	SetDialogueProgress(1);
	return true;
}

public func Dlg_Expert_Closed(object clonk)
{
	GameCall("OnHasTalkedExplosiveExpert", clonk);
	return true;
}

public func FxIntExpertStart(object target, proplist effect, int temp)
{
	if (temp)
		return FX_OK;
	return FX_OK;
}

public func FxIntExpertTimer(object target, proplist effect, int time)
{
	return FX_OK;
}
