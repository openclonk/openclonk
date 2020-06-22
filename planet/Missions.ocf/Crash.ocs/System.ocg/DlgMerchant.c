// NPC Merchant: Sells construction plans for clunker.

#appendto Dialogue

func Dlg_Merchant_1(object clonk)
{
	MessageBox("$Merchant1$", clonk);
	return true;
}

func Dlg_Merchant_2(object clonk)
{
	var options = [["$MerchantQPlane$", "Dlg_Merchant_Plane"]];
	var i = GetLength(options);
	if (g_has_bought_plans)
	{
		options[i++] = ["$MerchantQPump$", "Dlg_Merchant_Pump"];
		options[i++] = ["$MerchantQCatapult$", "Dlg_Merchant_Catapult"];
	}
	else
	{
		options[i++] = ["$MerchantQGold$", "Dlg_Merchant_Gold"];
		options[i++] = ["$MerchantQLake$", "Dlg_Merchant_Lake"];
	}
	options[i++] = ["$MerchantQDone$", "StopDialogue()"];
	MessageBox("", clonk, clonk, false, nil, options);
	SetDialogueProgress(1);
	return true;
}

func Dlg_Merchant_Lake(object clonk)
{
	if (g_has_bought_plans) return StopDialogue(); // in case multiple players initiate the dialogue at the same time
	MessageBox("$MerchantSellPlans$", clonk);
	SetDialogueProgress(10);
	return true;
}

func Dlg_Merchant_10(object clonk)
{
	if (g_has_bought_plans) return StopDialogue(); // in case multiple players initiate the dialogue at the same time
	var plr = clonk->GetOwner();
	var wealth = GetWealth(plr);
	if (wealth >= 150)
	{
		MessageBox("$MerchantBuyPlans$", clonk, clonk);
	}
	else
	{
		MessageBox("$MerchantNoMoney$", clonk, clonk);
		SetDialogueProgress(2);
	}
	return true;
}

func Dlg_Merchant_11(object clonk)
{
	// prevent race conditions
	if (g_has_bought_plans) return StopDialogue();
	var plr = clonk->GetOwner();
	var wealth = GetWealth(plr);
	if (wealth < 150)
	{
		MessageBox("$MerchantNoMoney$", clonk, clonk);
		SetDialogueProgress(2);
	}
	// do transaction.
	MessageBox("$MerchantGivePlans$", clonk);
	DoWealth(clonk->GetOwner(), -150);
	for (var i = 0; i < GetPlayerCount(); i++) GameCall("GiveExtraPlans", GetPlayerByIndex(i));
	g_has_bought_plans = true;
	SetDialogueProgress(2);
	return true;
}

func Dlg_Merchant_Plane(object clonk)
{
	MessageBox("$MerchantAPlane1$", clonk);
	SetDialogueProgress(20);
	return true;
}


func Dlg_Merchant_20(object clonk)
{
	MessageBox("$MerchantAPlane2$", clonk);
	SetDialogueProgress(2);
	return true;
}

func Dlg_Merchant_Pump(object clonk)
{
	MessageBox("$MerchantAPump1$", clonk);
	SetDialogueProgress(30);
	return true;
}


func Dlg_Merchant_30(object clonk)
{
	MessageBox("$MerchantAPump2$", clonk);
	SetDialogueProgress(2);
	return true;
}

func Dlg_Merchant_Catapult(object clonk)
{
	MessageBox("$MerchantACatapult1$", clonk);
	SetDialogueProgress(40);
	return true;
}


func Dlg_Merchant_40(object clonk)
{
	MessageBox("$MerchantACatapult2$", clonk, clonk);
	SetDialogueProgress(2);
	return true;
}

func Dlg_Merchant_Gold(object clonk)
{
	MessageBox("$MerchantAGold$", clonk);
	SetDialogueProgress(2);
	return true;
}