// Makes sure the guide message is hidden when starting a dialogue and shown again when closing.

#appendto Dialogue

public func Interact(object clonk)
{
	if (!dlg_interact || !dlg_name)
		return inherited(clonk, ...);
	var guide = FindObject(Find_ID(TutorialGuide), Find_Owner(clonk->GetOwner()));
	if (!guide)
		return inherited(clonk, ...);
	if (dlg_status == DLG_Status_Stop)
	{
		if (this.guide_was_shown)
		{
			this.guide_was_shown = false;
			guide->ShowGuide();				
		}
	}
	else if (dlg_status != DLG_Status_Remove && dlg_status != DLG_Status_Wait)
	{
		if (!guide->IsHidden())
		{
			this.guide_was_shown = true;
			guide->HideGuide();	
		}
	}
	return inherited(clonk, ...);
}