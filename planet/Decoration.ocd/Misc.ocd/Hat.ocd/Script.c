/*--- Hat ---*/

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Rebuy = true;

public func Definition(def) {
	SetProperty("PictureTransformation", Trans_Rotate(90, 0, 0, 1),def);
}

func GetCarryTransform(clonk)
{
	return Trans_Rotate(90, 0, 0, 1);
}
