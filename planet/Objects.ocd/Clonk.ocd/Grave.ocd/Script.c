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
	if(deathMessage == nil) deathMessage = "";
	PlayerMessage(clonk->GetController(), Format("$Epitaph$ %s.|%v", Contents(0)->~GetName(), Format("%s",deathMessage)));
}