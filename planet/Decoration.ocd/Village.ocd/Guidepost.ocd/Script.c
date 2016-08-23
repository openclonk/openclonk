/**
	@author Graphics Dustin Neß (dness.de), Script Sven2
*/

local Name="$Name$";
local Description="$Description$";

local inscription = "";

// Players can read the sign via the interaction bar.
public func IsInteractable() { return inscription != ""; }

// Adapt appearance in the interaction bar.
public func GetInteractionMetaInfo(object clonk)
{
	return { Description = "$MsgRead$", IconName = nil, IconID = GetID(), Selected = false };
}

// Called on player interaction.
public func Interact(object clonk)
{
	if (!clonk) return false;
	Dialogue->MessageBox(inscription, clonk, this, clonk->GetController(), true);
	return true;
}

public func SetInscription(to_text)
{
	inscription = to_text ?? "";
	return true;
}

// Color for messages
public func GetColor() { return 0xffcf9c1a; }

public func Construction()
{
	SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(RandomX(-180,180),0,10), Trans_Scale(360)));
}

public func Definition(def)
{
	if (!def.EditorProps) def.EditorProps = {};
	def.EditorProps.inscription = { Name="$Inscription$", Type="string", EditorHelp="$InscriptionHelp$", Set="SetInscription", Save="Inscription" };
}
