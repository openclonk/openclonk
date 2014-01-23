/**
	Gravestone
	Epitaph for the dead.
	
	@author Ringwaul
*/

public func IsInteractable() { return true; }

public func Interact(object clonk)
{
	if(!Contents(0) || !Contents(0)->~IsClonk()) return false;
	
	var deathMessage = Contents(0)->GetObjCoreDeathMessage();
	
	var graveInscription;
	if(deathMessage)
	{
		graveInscription = Format("$Epitaph$ %s.|\"%s\"", Contents(0)->~GetName(), deathMessage);
	} else {
		graveInscription = Format("$Epitaph$ %s.", Contents(0)->~GetName());
	}
	PlayerMessage(clonk->GetController(), graveInscription);
}