/**
	Attachment dummy
*/


/*-- Display --*/

public func GetCarryMode(object clonk, bool idle)
{
	return CARRY_Belt;
}

func Definition(def)
{
	SetProperty("PictureTransformation", Trans_Rotate(20, 1, 0, 1), def);
}
