/**
	Guidepost
	
	@author Graphics Dustin Neß (dness.de), Script Sven2
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
	if (!clonk) return false;
	Dialogue->MessageBox(GetTranslatedString(inscription), clonk, this, clonk->GetController(), true);
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
	// Pick an angle range that looks good (some angles show it from the side, which looks weird)
	var angle;
	if (!Random(2))
		angle = -80 + Random(105);
	else
		angle = 90 + Random(110);
	this.MeshTransformation = Trans_Mul(Trans_Rotate(angle, 0, 10), GetID().MeshTransformation);
}

public func Definition(proplist def)
{
	// Model file is way too large.
	def.MeshTransformation = Trans_Scale(360);
	def.PictureTransformation = Trans_Scale(900);
	// Inscription props.
	if (!def.EditorProps)
		def.EditorProps = {};
	def.EditorProps.inscription = { Name = "$Inscription$", Type = "string", EditorHelp = "$InscriptionHelp$", Set = "SetInscription", Save = "Inscription", Translatable = true };
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
