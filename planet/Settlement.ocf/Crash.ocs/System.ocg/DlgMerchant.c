// NPC Merchant: Sells construction plans for clunker.

#appendto Dialogue


private func Dlg_Merchant_1(object clonk)
{
	MessageBox("$MsgSellPlans$", clonk);
	return;
}

private func Dlg_Merchant_2(object clonk)
{
	var plr = clonk->GetOwner();
	var wealth = GetWealth(plr);
	if (wealth >= 150)
	{
		MessageBox("$AnsBuyPlans$", clonk, clonk);	
	}
	else
	{
		MessageBox("$AnsNoMoney$", clonk, clonk);
		SetDialogueProgress(0);
		SetDialogueStatus(DLG_Status_Stop);
	}
	return;
}

private func Dlg_Merchant_3(object clonk)
{
	MessageBox("$MsgGivePlans$", clonk);
	DoWealth(clonk->GetOwner(), -150);
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var plr = GetPlayerByIndex(i);
		SetPlrKnowledge(plr, Pump);
		SetPlrKnowledge(plr, Pipe);
		SetPlrKnowledge(plr, Catapult);
		SetPlrKnowledge(plr, Cannon);
	}	
	SetDialogueStatus(DLG_Status_Stop);
	return;
}

private func Dlg_Merchant_4(object clonk)
{
	MessageBox("$MsgLeaveVillage$", clonk);
	SetDialogueStatus(DLG_Status_Stop);
	return;
}
