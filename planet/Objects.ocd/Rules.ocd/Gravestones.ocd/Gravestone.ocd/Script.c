/**
	Gravestone
	Epitaph for the dead.
	
	@author Ringwaul
*/


local grave_inscription;

// Set the inscription for a dead clonk.
public func SetInscription(object dead)
{
	var death_message = dead->GetObjCoreDeathMessage();
	// Set grave inscription dependent on whether there is a death message.
	if (death_message)
		grave_inscription = Format("$Epitaph$ %s.|\"%s\"", dead->GetName(), death_message);
	else 
		grave_inscription = Format("$Epitaph$ %s.", dead->GetName());
	return;
}

// Set the inscription message directly.
public func SetInscriptionMessage(string message)
{
	grave_inscription = message;
	return;
}
 
public func IsInteractable() { return true; }

public func GetInteractionMetaInfo(object clonk)
{
	return { Description = "$ReadInscription$" };
}

public func Interact(object clonk)
{
	return PlayerMessage(clonk->GetController(), grave_inscription);
}

/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";