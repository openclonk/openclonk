/**
	Stone Sign
	Is attached to a wall and has an inscription.
	
	@author Maikel
*/


local inscription = "";

// Players can read the sign via the interaction bar.
public func IsInteractable() { return inscription != ""; }

// Adapt appearance in the interaction bar.
public func GetInteractionMetaInfo(object clonk)
{
	return { Description = "$MsgRead$", Selected = false };
}

// Called on player interaction.
public func Interact(object clonk)
{
	if (!clonk)
		return false;
	Dialogue->MessageBox(GetTranslatedString(inscription), clonk, this, clonk->GetController(), true);
	return true;
}

public func SetInscription(to_text)
{
	inscription = to_text ?? "";
	return true;
}

// Color for messages.
public func GetColor() { return 0xffb0b0b0; }

public func Definition(def)
{
	// Inscription editor props.
	if (!def.EditorProps)
		def.EditorProps = {};
	def.EditorProps.inscription = { Name="$Inscription$", Type="string", EditorHelp="$InscriptionHelp$", Set="SetInscription", Save="Inscription", Translatable=true };
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local MeshTransformation = [0, 0, 1000, -5600, 0, 1000, 0, -300, -1000, 0, 0, 0]; // Trans_Mul(Trans_Translate(-5600, -300), Trans_Rotate(90, 0, 1, 0))
local PictureTransformation = [0, 0, 1000, 0, 0, 1000, 0, 0, -1000, 0, 0, 0]; // Trans_Rotate(90, 0, 1, 0)