/*--
		QuickBuy
--*/

global func PlayerControl(int plr, int ctrl)
{
	if (ctrl >= CON_QuickBuy0 && ctrl <= CON_QuickBuy9)
	{
		if (!g_homebases || plr < 0) return false;
		var base = g_homebases[plr];
		if (!base) return false;
		base->QuickBuyItem(g_quickbuy_items[ctrl - CON_QuickBuy0]);
		return true;
	}
	if (ctrl == CON_ToggleShop)
	{
		var buy_menu = FindObject(Find_ID(GUI_BuyMenu), Find_Owner(plr));
		if (!buy_menu) return false;
		return buy_menu->ToggleVisibility(plr);
	}
	return _inherited(plr, ctrl, ...);
}