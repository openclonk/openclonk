#appendto GoldBar

local value;

public func SetValue(int new_value)
{
	value = new_value;
	UpdatePicture();
}

// Give the player picking up the bar extra money
public func RejectEntrance(object container)
{
	if (!container->~IsClonk()) return false;
	var plr = container->GetOwner();
	if (GetPlayerType(plr) != C4PT_User) return false;

	DoWealth(plr, value);
	Sound("UI::Cash", false, nil, plr);
	if (g_chest && !g_chest->FindContents(GoldBar))
		g_chest->SetMeshMaterial("DefaultChest", 0);
	RemoveObject();
	return true;
}

private func UpdatePicture()
{
	var one = value % 10;
	var ten = (value / 10) % 10;
	var hun = (value / 100) % 10;

	var s = 400;
	var yoffs = 14000;
	var xoffs = 22000;
	var spacing = 14000;

	if (hun > 0)
	{
		SetGraphics(Format("%d", hun), Icon_Number, 10, GFXOV_MODE_Picture);
		SetObjDrawTransform(s, 0, xoffs - 2 * spacing, 0, s, yoffs, 10);
	}
	else
		SetGraphics(nil, nil, 10);

	if (ten > 0 || hun > 0)
	{
		SetGraphics(Format("%d", ten), Icon_Number, 11, GFXOV_MODE_Picture);
		SetObjDrawTransform(s, 0, xoffs - spacing, 0, s, yoffs, 11);
	}
	else
		SetGraphics(nil, nil, 11);

	SetGraphics(Format("%d", one), Icon_Number, 12, GFXOV_MODE_Picture);
	SetObjDrawTransform(s, 0, xoffs, 0, s, yoffs, 12);
	return;
}