// The Skyville head.

#appendto Dialogue

public func Dlg_SkyvilleHead_Init(object clonk)
{
	return true;
}

public func Dlg_SkyvilleHead_1(object clonk)
{
	MessageBox("$DlgSkyvilleHeadThanks$", clonk, dlg_target);
	return true;
}

public func Dlg_SkyvilleHead_2(object clonk)
{
	MessageBox("$DlgSkyvilleHeadDoBest$", clonk, clonk);
	return true;
}

public func Dlg_SkyvilleHead_3(object clonk)
{
	MessageBox("$DlgSkyvilleHeadManage$", clonk, dlg_target);
	StopDialogue();
	SetDialogueProgress(1);
	return true;
}

public func Dlg_SkyvilleHead_Closed(object clonk)
{
	return true;
}
