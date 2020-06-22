/**
	Gravestone
	Epitaph for the dead.
	
	@author Ringwaul
*/


local grave_inscription;

// Definition call: Get death message for clonk
public func GetInscriptionForClonk(object dead)
{
	var msg = dead->GetObjCoreDeathMessage();
	if (!msg) msg = dead.SpecialDeathMessage;
	// Set grave inscription dependent on whether there is a death message.
	if (msg && GetLength(msg))
		msg = Format("$Epitaph$ %s.|\"%s\"", dead->GetName(), msg);
	else 
		msg = Format("$Epitaph$ %s.", dead->GetName());
	return msg;
}

// Set the inscription for a dead clonk.
public func SetInscription(object dead)
{
	grave_inscription = GetInscriptionForClonk(dead);
	return true;
}

// Set the inscription message directly.
public func SetInscriptionMessage(message)
{
	grave_inscription = message;
	return true;
}
 
public func IsInteractable() { return true; }

public func GetInteractionMetaInfo(object clonk)
{
	return { Description = "$ReadInscription$" };
}

public func Interact(object clonk)
{
	return PlayerMessage(clonk->GetController(), GetTranslatedString(grave_inscription));
}

public func Definition(def)
{
	if (!def.EditorProps) def.EditorProps = {};
	def.EditorProps.grave_inscription = { Name="$Inscription$", Type="string", EditorHelp="$InscriptionHelp$", Set="SetInscriptionMessage", Save="Inscription", Translatable = true };
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Plane = 300;
