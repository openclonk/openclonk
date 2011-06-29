// The player's clonk is not allowed to collect certain items.

#appendto Clonk

protected func RejectCollect(id def, object obj)
{
	if (GetOwner() == 0)
		if (def == Arrow || def == Bow || def == Javelin)
			return true;
	return _inherited(def, obj, ...);
}