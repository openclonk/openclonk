/* --- OC specific interaction menu --- */

// Clonks act as containers for the interaction menu as long as they are alive.
public func IsContainer() { return GetAlive(); }

// You can not interact with dead Clonks.
// This would be the place to show a death message etc.
public func RejectInteractionMenu(object to)
{
	if (!GetAlive())
		return Format("$MsgDeadClonk$", GetName());
	return _inherited(to, ...);
}

// You can not display the Clonk as a content entry in a building.
// Otherwise you can transfer a crew member to your inventory...
public func RejectInteractionMenuContentEntry(object menu_target, object container)
{
	return true;
}

public func GetSurroundingEntryMessage(object for_clonk)
{
	if (!GetAlive()) return Format("{{Clonk_Grave}} %s", Clonk_Grave->GetInscriptionForClonk(this));
}


/* Enable the Clonk to pick up stuff from its surrounding in the interaction menu */
public func OnInteractionMenuOpen(object menu)
{
	_inherited(menu, ...);
	
	// Allow picking up stuff from the surrounding only if not in a container itself.
	if (!Contained())
	{
		var surrounding = CreateObject(Helper_Surrounding);
		surrounding->InitFor(this, menu);
	}
}
