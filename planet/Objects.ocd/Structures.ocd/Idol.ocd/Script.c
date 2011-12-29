protected func Initialize()
{
	SetCategory(C4D_Vehicle);
	return _inherited(...);
}

private func MenuOnInteraction() { return false; }
private func MenuOnControlUse() { return false; }
private func MenuOnControlUseAlt() { return false; }

local Name = "$Name$";
local Touchable = 1;
