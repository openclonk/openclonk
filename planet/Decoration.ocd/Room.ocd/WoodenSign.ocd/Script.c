/**
	Wooden Sign
	Is attached to a wall and has an inscription.
	
	@author Maikel, Foaly (graphics)
*/

#include EnvPack_Guidepost


public func Construction()
{
	return;
}

public func Definition(def)
{
	// Inscription editor props.
	if (!def.EditorProps)
		def.EditorProps = {};
	def.EditorProps.inscription = { Name="$Inscription$", Type="string", EditorHelp="$InscriptionHelp$", Set="SetInscription", Save="Inscription", Translatable = true };
	def.MeshTransformation = Trans_Mul(Trans_Translate(-600, -400), Trans_Rotate(90, 0, 1, 0),Trans_Scale(900));
	def.PictureTransformation = Trans_Rotate(90, 0, 1, 0);
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";