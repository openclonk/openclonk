/**
	Guidepost
	
	@author Sven2, Nachtfalter (graphics)
*/

#include EnvPack_Guidepost


public func Construction(...)
{
	// Pick an angle range that looks good (some angles show it from the side or back, which looks weird).
	this.MeshTransformation = Trans_Mul(Trans_Rotate(-60 + Random(90), 0, 10), GetID().MeshTransformation);
	return;
}

public func Definition(proplist def)
{
	_inherited(def);
	// Model file is way too large.
	def.MeshTransformation = Trans_Scale(130);
	def.PictureTransformation = Trans_Mul(Trans_Translate(-5000, -60000, 150000), Trans_Rotate(-20, 0, 1), Trans_Scale(400));
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";