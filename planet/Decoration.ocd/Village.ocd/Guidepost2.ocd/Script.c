/**
	@author Nachtfalter
*/

#include EnvPack_Guidepost

local Name="$Name$";
local Description="$Description$";

protected func Construction(...)
{
	// Pick an angle range that looks good (some angles show it from the side, which looks weird)
	var angle;
	if (!Random(2)) angle = -60 + Random(90); else angle = 120 + Random(90);
	this.MeshTransformation = Trans_Mul(Trans_Rotate(angle,0,10), GetID().MeshTransformation);
	return _inherited(...);
}

public func Definition(def)
{
	_inherited(def);
	// Model file is way too large
	def.MeshTransformation = Trans_Scale(130);
}
