/**
	Stone Sign
	Is attached to a wall and has an inscription.
	
	@author Maikel, Foaly (graphics)
*/

#include EnvPack_Guidepost


public func Construction()
{
	return;
}

// Color for messages.
public func GetColor() { return 0xffb0b0b0; }

public func Definition(proplist def)
{
	// Inscription editor props.
	if (!def.EditorProps)
		def.EditorProps = {};
	def.EditorProps.inscription = { Name="$Inscription$", Type="string", EditorHelp="$InscriptionHelp$", Set="SetInscription", Save="Inscription", Translatable=true };
	def.MeshTransformation = Trans_Mul(Trans_Translate(-5600, -300), Trans_Rotate(90, 0, 1, 0));
	def.PictureTransformation = Trans_Rotate(90, 0, 1, 0);
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";